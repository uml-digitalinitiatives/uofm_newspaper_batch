<?php

class UoMNewspaperBatchPreprocessor extends IslandoraBookBatch {
  protected static function get_object_class() {
    return 'UoMNewspaperPaperObject';
  }

  /**
   * Should be able to find all that we need just from the unit.xml files.
   */
  protected static function get_pattern() {
    return '/unit\.xml/';
  }

  protected function re_group(&$files, $components, $relative_uri, $file_info) {
    // TODO: Use the three last components (representing the date) as the base name
    $base = implode('-', array_slice($components, -3));
    $files[$base]['.'][$file_info->name] = $file_info;
  }
}

/**
 * Attempt to reparse a string into a more easily usable format.
 *
 * @param string $date_string_in
 *   A date we will pass to strtotime() to try to get a timestamp.
 *
 * @return string
 *   A string of the format YYYY-MM-DD.
 */
function uofm_newspaper_batch_fix_date($date_string_in) {
  $timestamp = strtotime((string)$date_string_in);
  $date_string_out = date('Y-m-d', $timestamp);
  return $date_string_out;
}

/**
 * Fix the couple date elements into a format parsable by our GSearch XSLT.
 *
 * @param SimpleXMLElement $mods
 *   A SimpleXMLElement containing a MODS record.
 */
function uom_newspaper_batch_fix_dates(SimpleXMLElement $mods) {
  // The fields in to change are /m:mods/m:originInfo/m:dateIssued and
  //   /m:mods/m:relatedItem[@type="host"]/m:part/m:date
  $mods->registerXPathNamespace('m', 'http://www.loc.gov/mods/v3');
  $to_reformat = $mods->xpath('/m:mods/m:originInfo/m:dateIssued |
    /m:mods/m:relatedItem[@type="host"]/m:part/m:date');
  if ($to_reformat) {
    foreach ($to_reformat as $date) {
      $date->{0} = uofm_newspaper_batch_fix_date((string)$date);
      $date['encoding'] = 'iso8601';
    }
  }
}

class UoMNewspaperPaperObject extends IslandoraFlatBatchObject {
  public function initial_batch_state() {
    return ISLANDORA_BATCH_STATE__NOT_READY;
  }

  private function get_unit() {
    $unit = NULL;

    if ($unit === NULL) {
      $unit = simplexml_load_file($this->object_info['.']['unit']->uri);
      $unit->registerXPathNamespace('ia', 'http://www.iarchives.com/schema/2002/export');
    }

    return $unit;
  }

  public function get_children(IslandoraTuque $connection) {
    // Iterate over page entries in unit.xml, creating children.
    $unit = $this->get_unit();

    $children = array();

    // XXX: drupal_dirname() may not work with ZIP sources...
    $unit_dirname = drupal_dirname($this->object_info['.']['unit']->uri);

    $i = 0;
    foreach ($unit->pages->{'page-ref'} as $page_ref) {
      // Synthesize the "object_info" structure.
      $page_obj = new stdClass();
      $page_obj->uri = $unit_dirname . '/' . $page_ref['href'];
      $page_obj->filename = pathinfo($page_obj->uri, PATHINFO_BASENAME);
      $page_obj->name = pathinfo($page_obj->uri, PATHINFO_FILENAME);

      $page_object_info = array(
        '.' => array(
          'page' => $page_obj,
        ),
      );
      $children[] = new UoMNewspaperPageObject($connection, $this->id, $i++,
        $page_object_info, $this->preprocessor_parameters);
    }

    return $children;
  }

  protected function get_mods() {
    // Process the unit.xml into MODS (if we don't already have MODS).
    if (!isset($this['MODS'])) {
      $mods = $this->constructDatastream('MODS', 'M');
      $mods->mimetype = 'text/xml';
      $mods->label = 'MODS Record';
      $mods_simplexml = new SimpleXMLElement(static::run_xsl_transform(array(
        'input' => file_get_contents($this->object_info['.']['unit']->uri),
        'xsl' => drupal_get_path('module', 'uofm_newspaper_batch') . '/transforms/iarchive_to_mods.xsl',
      )));
      uom_newspaper_batch_fix_dates($mods_simplexml);
      $mods->content = $mods_simplexml->saveXML();

      if (isset($mods->content)) {
        $this->ingestDatastream($mods);
      }
    }

    // Allow fall-back to the parent implementation.
    return parent::get_mods();
  }

  public function batch_process() {
    // Should add the "unit.xml" as "unit".
    $parent_return = parent::batch_process();

    // Add the article XML entries onto the newspaper object.
    $articles = $this->get_unit()->xpath('/ia:unit/ia:articles/ia:article-ref');
    if ($articles) {
      $unit_dir = dirname($this->object_info['.']['unit']->uri);
      foreach(array_values($articles) as $article) {
        $article_file = $unit_dir . '/' . $article['href'];
        $article_number = pathinfo($article_file, PATHINFO_FILENAME);
        $article_ds = $this->constructDatastream("ARTICLE-$article_number", 'M');
        $article_ds->label = "Article $article_number";
        $article_ds->mimetype = 'application/xml';
        $article_ds->setContentFromFile($article_file, FALSE);

        $this->ingestDatastream($article_ds);
      }
    }

    return ISLANDORA_BATCH_STATE__DONE;
  }
}

class UoMNewspaperPageObject extends IslandoraFlatBatchObject {
  protected $parent_id;

  /**
   * Constructor.
   *
   *
   * @param IslandoraTuque $connection
   *   Something with a repository we can create an object in.
   * @param string $parent_id
   *   The newspaper to which we belong.
   * @param integer $sequece
   *   A sequence number... Just based on the order in which we've encountered
   *   this page, not necessarily it's order in the paper.
   * @param array $object_info
   *   An array, similar in structure to that handled by IslandoraFlatBatchObject.
   * @param array $pp
   *   The array of preprocessor options.
   */
  public function __construct(IslandoraTuque $connection, $parent_id, $sequence, $object_info, $pp) {
    parent::__construct($connection, "$parent_id-$sequence", $object_info, $pp);

    $this->parent_id = $parent_id;
  }
  public function initial_batch_state() {
    return ISLANDORA_BATCH_STATE__NOT_READY;
  }

  public function get_children(IslandoraTuque $connection) {
    // Pages have no children.
    return array();
  }

  public function batch_process() {
    // Should add the page.xml as "page".
    $parent_return = parent::batch_process();

    // Grab the TIFF (as OBJ).
    $this->add_tiff();
    // Grab the PDF for the page.
    $this->add_pdf();
    // Grab the whatever_sm.jpg version (as JPG)
    $this->add_sm_jpg();
    // Translate the page.xml to hOCR.
    $this->add_hocr();

    // XXX: Needs to be made to use the shared image derivative module/library
    //   (didn't actually exist at the time of writing).
    // Generate the JP2 and TN.
    $uploaded_file = $this->find_tiff();
    $base_name = str_replace(':', '-', $this->id);
    if ($uploaded_file) {
      module_load_include('inc', 'islandora_large_image', 'includes/derivatives');
      $jp2 = islandora_large_image_create_JP2_derivative($this, $uploaded_file, $base_name);
      $tn = islandora_large_image_create_TN_derivative($this, $uploaded_file, $base_name);
    }

    return ISLANDORA_BATCH_STATE__DONE;
  }

  /**
   * Add the tiff.
   *
   * @return FedoraDatastream
   *   The datastream which has been added to this object.
   */
  protected function add_tiff() {
    if (!isset($this['OBJ'])) {
      // Add the tiff (if it isn't already on the object).
      $obj = $this->constructDatastream('OBJ', 'M');
      $obj->label = 'Newspaper page';
      $obj->mimetype = 'image/tiff';
      $obj->setContentFromFile($this->find_tiff(), FALSE);

      $this->ingestDatastream($obj);
    }

    return $this['OBJ'];
  }

  private function find_tiff() {
    $path = NULL;

    if ($path === NULL) {
      // Get the URI of the page.xml
      $page_uri = $this->object_info['.']['page']->uri;
      // Break into components.
      $components = explode('/', $page_uri);

      // Add in component for TIFF directory
      array_splice($components, -5, 0, 'TIFFS');
      // Change "pages" to "Archival_Images"
      array_splice($components, -2, 1, 'Archival_Images');

      // Change the file extension.
      $xml = array_pop($components);
      $components[] = pathinfo($xml, PATHINFO_FILENAME) . '.tif';

      $path = implode('/', $components);
    }

    return $path;
  }

  /**
   * Add the pdf.
   *
   * @return FedoraDatastream
   *   The datastream which has been added to this object.
   */
  protected function add_pdf() {
    if (!isset($this['PDF'])) {
      // Add the pdf (if it isn't already on the object).
      $obj = $this->constructDatastream('PDF', 'M');
      $obj->label = 'Newspaper page PDF';
      $obj->mimetype = 'application/pdf';
      $obj->setContentFromFile($this->find_pdf(), FALSE);

      $this->ingestDatastream($obj);
    }

    return $this['PDF'];
  }

  private function find_pdf() {
    $path = NULL;

    if ($path === NULL) {
      // Get the URI of the page.xml
      $page_uri = $this->object_info['.']['page']->uri;
      // Break into components.
      $components = explode('/', $page_uri);

      // Add in "pdf" component.
      array_splice($components, -1, 0, 'pdf');

      // Change the file extension.
      $xml = array_pop($components);
      $components[] = pathinfo($xml, PATHINFO_FILENAME) . '.pdf';

      $path = implode('/', $components);
    }

    return $path;
  }

  /**
   * Add the sm_jpg.
   *
   * @return FedoraDatastream
   *   The datastream which has been added to this object.
   */
  protected function add_sm_jpg() {
    if (!isset($this['JPG'])) {
      // Add the sm_jpg (if it isn't already on the object).
      $obj = $this->constructDatastream('JPG', 'M');
      $obj->label = 'Newspaper page';
      $obj->mimetype = 'image/jpeg';
      $obj->setContentFromFile($this->find_sm_jpg(), FALSE);

      $this->ingestDatastream($obj);
    }

    return $this['JPG'];
  }

  private function find_sm_jpg() {
    $path = NULL;

    if ($path === NULL) {
      // Get the URI of the page.xml
      $page_uri = $this->object_info['.']['page']->uri;
      // Break into components.
      $components = explode('/', $page_uri);

      // Add in "images" component.
      array_splice($components, -1, 0, 'images');

      // Change the file extension.
      $xml = array_pop($components);
      $components[] = pathinfo($xml, PATHINFO_FILENAME) . '_sm.jpg';

      $path = implode('/', $components);
    }

    return $path;
  }

  /**
   * They want the label to be the page number.
   */
  protected function get_title() {
    return $this->get_page_number();
  }

  protected function get_page_number() {
    $page_number = NULL;
    // Return the page/sequence number.
    if ($page_number === NULL) {
      $p = $this->get_page()->xpath('/ia:page/ia:header-item[@name="page"]');
      if (count($p) > 0) {
        $page_number = $p[0]['value'];
      }
    }

    return $page_number;
  }


  protected function add_hocr() {
    // Process the page.xml into MODS (if we don't already have MODS).
    if (!isset($this['HOCR'])) {
      $hocr = $this->constructDatastream('HOCR', 'M');
      $hocr->mimetype = 'text/xml';
      $hocr->label = 'HOCR Record';
      $hocr->content = static::run_xsl_transform(array(
            'input' => file_get_contents($this->object_info['.']['page']->uri),
            'xsl' => drupal_get_path('module', 'uofm_newspaper_batch') . '/transforms/iarchive_to_hocr.xsl',
            ));

      if (isset($hocr->content)) {
        $this->ingestDatastream($hocr);
      }
    }

    // Allow fall-back to the parent implementation.
    return $this['HOCR'];
  }

  protected function get_mods() {
    // Process the page.xml into MODS (if we don't already have MODS).
    if (!isset($this['MODS'])) {
      $mods = $this->constructDatastream('MODS', 'M');
      $mods->mimetype = 'text/xml';
      $mods->label = 'MODS Record';
      $mods_simplexml = new SimpleXMLElement(static::run_xsl_transform(array(
        'input' => file_get_contents($this->object_info['.']['page']->uri),
        'xsl' => drupal_get_path('module', 'uofm_newspaper_batch') . '/transforms/iarchive_to_mods.xsl',
      )));
      uom_newspaper_batch_fix_dates($mods_simplexml);
      $mods->content = $mods_simplexml->saveXML();

      if (isset($mods->content)) {
        $this->ingestDatastream($mods);
      }
    }

    // Allow fall-back to the parent implementation.
    return parent::get_mods();
  }

  /**
   * Get the page XML, parsed with SimpleXML.
   */
  private function get_page() {
    $page = NULL;

    if ($page === NULL) {
      $page = simplexml_load_file($this->object_info['.']['page']->uri);
      $page->registerXPathNamespace('ia', 'http://www.iarchives.com/schema/2002/export');
    }

    return $page;
  }

  public function add_relationships() {
    $this->models = "islandora:newspaperPageCModel";
    $this->relationships->add(ISLANDORA_RELS_EXT_URI, 'isMemberOf', $this->parent_id);
    $this->relationships->add(ISLANDORA_RELS_EXT_URI, 'isSequenceNumber', $this->get_page_number(), TRUE);
  }
}

