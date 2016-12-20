<?php

/**
 * @file
 * Classes and functions related to the islandora_batch framework.
 */

class UoMIarchivesNewspaperBatchPreprocessor extends IslandoraBookBatch {
  /**
   * Inherits.
   */
  protected static function getObjectClass() {
    return 'UoMIarchivesNewspaperPaperObject';
  }

  /**
   * Should be able to find all that we need just from the unit.xml files.
   */
  protected static function getPattern() {
    return '/unit\.xml/';
  }

  /**
   * Inherits.
   */
  protected function reGroup(&$files, $components, $relative_uri, $file_info) {
    // Use the three last components (representing the date) as the base name.
    $base = implode('-', array_slice($components, -3));
    $files[$base]['.'][$file_info->name] = $file_info;
  }
}

class UoMIarchivesNewspaperPaperObject extends UoMNewspaperPaperObject {
  /**
   * Inherits.
   */
  public function initialBatchState() {
    return ISLANDORA_BATCH_STATE__NOT_READY;
  }

  /**
   * Get the unit XML, parsed with simple XML.
   */
  protected function getUnit() {
    $unit = NULL;

    if ($unit === NULL) {
      $unit = simplexml_load_file($this->objectInfo['.']['unit']->uri);
      if ($unit) {
        $unit->registerXPathNamespace('ia', 'http://www.iarchives.com/schema/2002/export');
      }
      else {
        throw new Exception(t('Failed to parse XML "@file".', array(
          '@file' => $this->objectInfo['.']['unit']->uri,
        )));
      }
    }

    return $unit;
  }

  /**
   * Inherits.
   */
  public function getChildren(IslandoraTuque $connection) {
    // Iterate over page entries in unit.xml, creating children.
    $unit = $this->getUnit();

    $children = array();

    // XXX: drupal_dirname() may not work with ZIP sources...
    $unit_dirname = drupal_dirname($this->objectInfo['.']['unit']->uri);
    $title = $this->getUnit()['collection-title'];

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
        'title' => (string) $title,
      );
      $children[] = new UoMIarchivesNewspaperPageObject($connection, $this->id, $i++,
        $page_object_info, $this->preprocessorParameters);
    }

    return $children;
  }

  /**
   * Inherits.
   */
  protected function getMods() {
    // Process the unit.xml into MODS (if we don't already have MODS).
    if (!isset($this['MODS'])) {
      $mods = $this->constructDatastream('MODS', 'M');
      $mods->mimetype = 'text/xml';
      $mods->label = 'MODS Record';

      $mods->content = static::runXslTransform(array(
        'input' => file_get_contents($this->objectInfo['.']['unit']->uri),
        'xsl' => drupal_get_path('module', 'uofm_newspaper_batch') . '/transforms/iarchive_to_mods.xsl',
        'php_functions' => array(
          'uofm_newspaper_batch_fix_date',
        ),
      ));

      if (isset($mods->content)) {
        $this->ingestDatastream($mods);
      }
    }

    // Allow fall-back to the parent implementation.
    return parent::getMods();
  }

  /**
   * Inherits.
   */
  public function getResources() {
    return array(
      'iarchive_file_path' => array(
        $this->objectInfo['.']['unit']->uri,
      ),
    );
  }

  /**
   * Inherits.
   */
  public function addRelationships() {
    parent::addRelationships();
    // If we don't know the order set them all to 1
    $this->relationships->add(ISLANDORA_RELS_EXT_URI, 'isSequenceNumber', 1, TRUE);
  }

  /**
   * Inherits.
   */
  public function batchProcess() {
    // Should add the "unit.xml" as "unit".
    $parent_return = parent::batchProcess();

    // Add the article XML entries onto the newspaper object.
    $articles = $this->getUnit()->xpath('/ia:unit/ia:articles/ia:article-ref');
    if ($articles) {
      $unit_dir = dirname($this->objectInfo['.']['unit']->uri);
      foreach (array_values($articles) as $article) {
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

class UoMIarchivesNewspaperPageObject extends UoMNewspaperPaperObject {
  protected $parentId;

  /**
   * Constructor.
   *
   *
   * @param IslandoraTuque $connection
   *   Something with a repository we can create an object in.
   * @param string $parent_id
   *   The newspaper to which we belong.
   * @param int $sequence
   *   A sequence number... Just based on the order in which we've encountered
   *   this page, not necessarily it's order in the paper.
   * @param array $object_info
   *   An array, similar in structure to that handled by
   *   IslandoraFlatBatchObject.
   * @param array $pp
   *   The array of preprocessor options.
   */
  public function __construct(IslandoraTuque $connection, $parent_id, $sequence, $object_info, $pp) {
    parent::__construct($connection, "$parent_id-$sequence", $object_info, $pp);

    $this->parentId = $parent_id;
  }

  /**
   * Inherits.
   */
  public function initialBatchState() {
    return ISLANDORA_BATCH_STATE__NOT_READY;
  }

  /**
   * Inherits.
   */
  public function getChildren(IslandoraTuque $connection) {
    // Pages have no children.
    return array();
  }

  /**
   * Inherits.
   */
  public function batchProcess() {
    // Should add the page.xml as "page".
    $parent_return = parent::batchProcess();

    // Grab the TIFF (as OBJ).
    $this->addTiff();
    // Grab the PDF for the page.
    $this->addPdf();
    // Grab the whatever_sm.jpg version (as JPG).
    // Small JPGs no longer exist by default.
    // $this->addSmJpg();
    // Translate the page.xml to hOCR.
    if (!$this->preprocessorParameters['generate_ocr']) {
      $this->addHocr();
      $this->addOcr();
    }

    return ISLANDORA_BATCH_STATE__DONE;
  }

  /**
   * Add the tiff.
   *
   * @return AbstractDatastream
   *   The datastream which has been added to this object.
   */
  protected function addTiff() {
    if (!isset($this['OBJ'])) {
      // Add the tiff (if it isn't already on the object).
      $obj = $this->constructDatastream('OBJ', 'M');
      $obj->label = 'Newspaper page';
      $obj->mimetype = 'image/tiff';
      $obj->setContentFromFile($this->findTiff(), FALSE);

      $this->ingestDatastream($obj);
    }

    return $this['OBJ'];
  }

  protected $tiffPath = NULL;
  /**
   * Build the path to where the TIFF is (should be) located.
   */
  protected function findTiff() {
    if ($this->tiffPath === NULL) {
      // Get the URI of the page.xml.
      $page_uri = $this->objectInfo['.']['page']->uri;
      // Break into components.
      $components = explode('/', $page_uri);

      // Add in component for TIFF directory.
      // Below was only needed for the ones I had sorted.
      // array_splice($components, -5, 0, 'TIFFS');
      // Change "pages" to "Archival_Images".
      array_splice($components, -2, 1, 'Archival_Images');

      // Change the file extension.
      $xml = array_pop($components);
      $components[] = pathinfo($xml, PATHINFO_FILENAME) . '.tif';

      $this->tiffPath = implode('/', $components);
    }

    return $this->tiffPath;
  }

  /**
   * Add the pdf.
   *
   * @return AbstractDatastream
   *   The datastream which has been added to this object.
   */
  protected function addPdf() {
    if (!isset($this['PDF'])) {
      // Add the pdf (if it isn't already on the object).
      $pdf = $this->findPdf();
      if (!is_null($pdf)) {
        $obj = $this->constructDatastream('PDF', 'M');
        $obj->label = 'Newspaper page PDF';
        $obj->mimetype = 'application/pdf';
        $obj->setContentFromFile($pdf, FALSE);
        $this->ingestDatastream($obj);
      }
    }

    return $this['PDF'];
  }

  /**
   * Build the path to the page PDF.
   */
  protected function findPdf() {
    $path = NULL;

    if ($path === NULL) {
      // Get the URI of the page.xml
      $page_uri = $this->objectInfo['.']['page']->uri;
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
   * @return AbstractDatastream
   *   The datastream which has been added to this object.
   */
  protected function addSmJpg() {
    if (!isset($this['JPG'])) {
      // Add the sm_jpg (if it isn't already on the object).
      $jpg = $this->findSmJpg();
      if (!is_null($jpg)) {
        $obj = $this->constructDatastream('JPG', 'M');
        $obj->label = 'Newspaper page';
        $obj->mimetype = 'image/jpeg';
        $obj->setContentFromFile($jpg, FALSE);
        $this->ingestDatastream($obj);
      }
    }

    return $this['JPG'];
  }

  /**
   * Build the path to the small JPG image.
   */
  protected function findSmJpg() {
    $path = NULL;

    if ($path === NULL) {
      // Get the URI of the page.xml
      $page_uri = $this->objectInfo['.']['page']->uri;
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
   * Get the page number of this page from the page XML.
   */
  protected function getPageNumber() {
    $page_number = NULL;
    // Return the page/sequence number.
    if ($page_number === NULL) {
      $p = $this->getPage()->xpath('/ia:page/ia:header-item[@name="page"]');
      if (count($p) > 0) {
        $page_number = $p[0]['value'];
      }
    }

    return $page_number;
  }

  /**
   * Transform the page XML to produce HOCR similar to that from Tesseract.
   */
  protected function addHocr() {
    // Process the page.xml into MODS (if we don't already have MODS).
    if (!isset($this['HOCR'])) {
      $hocr = $this->constructDatastream('HOCR', 'M');
      $hocr->mimetype = 'text/xml';
      $hocr->label = 'HOCR Record';

      $base_hocr = static::runXslTransform(array(
        'input' => file_get_contents($this->objectInfo['.']['page']->uri),
        'xsl' => drupal_get_path('module', 'uofm_newspaper_batch') . '/transforms/iarchive_to_hocr.xsl',
      ));

      list($width, $height) = getimagesize($this->findTiff());

      $xml = new SimpleXMLElement($base_hocr);
      $xml->registerXPathNamespace('x', 'http://www.w3.org/1999/xhtml');
      $pages = $xml->xpath('/x:html/x:body/x:div[@class="ocr_page"][1]');
      if ($pages) {
        // Grab the page.
        $page = reset($pages);
        $page['title'] = format_string('image "!image"; bbox 0 0 !width !height; ppageno 0', array(
          '!image' => $this->findTiff(),
          '!width' => $width,
          '!height' => $height,
        ));
      }

      $hocr->content = $xml->asXML();
      if (isset($hocr->content)) {
        $this->ingestDatastream($hocr);
      }
    }

    return $this['HOCR'];
  }

  /**
   * Transform the page XML to produce flat text.
   */
  protected function addOcr() {
    // Process the page.xml into MODS (if we don't already have MODS).
    if (!isset($this['OCR'])) {
      $ocr = $this->constructDatastream('OCR', 'M');
      $ocr->mimetype = 'text/plain';
      $ocr->label = 'OCR Record';

      $base_hocr = static::runXslTransform(array(
        'input' => file_get_contents($this->objectInfo['.']['page']->uri),
        'xsl' => drupal_get_path('module', 'uofm_newspaper_batch') . '/transforms/iarchive_to_hocr.xsl',
      ));

      $ocr->content = static::runXslTransform(array(
        'input' => $base_hocr,
        'xsl' => drupal_get_path('module', 'uofm_newspaper_batch') . '/transforms/flatten_xml_text_nodes.xslt',
      ));

      if (isset($ocr->content)) {
        $this->ingestDatastream($ocr);
      }
    }

    return $this['OCR'];
  }

  /**
   * Inherits.
   */
  protected function getMods() {
    // Process the page.xml into MODS (if we don't already have MODS).
    if (!isset($this['MODS'])) {
      $mods = $this->constructDatastream('MODS', 'M');
      $mods->mimetype = 'text/xml';
      $mods->label = 'MODS Record';
      $mods->content = static::runXslTransform(array(
        'input' => file_get_contents($this->objectInfo['.']['page']->uri),
        'xsl' => drupal_get_path('module', 'uofm_newspaper_batch') . '/transforms/iarchive_to_mods.xsl',
        'php_functions' => array(
          'uofm_newspaper_batch_fix_date',
        ),
        'parameters' => array(
          '' => array(
            'paperTitle' => $this->objectInfo['title'],
          ),
        ),
      ));

      if (isset($mods->content)) {
        $this->ingestDatastream($mods);
      }
    }

    // Allow fall-back to the parent implementation.
    return parent::getMods();
  }

  /**
   * Get the page XML, parsed with SimpleXML.
   */
  protected function getPage() {
    $page = NULL;

    if ($page === NULL) {
      $page = simplexml_load_file($this->objectInfo['.']['page']->uri);
      if ($page) {
        $page->registerXPathNamespace('ia', 'http://www.iarchives.com/schema/2002/export');
      }
      else {
        throw new Exception(t('Failed to parse the XML file "@file".', array(
          '@file' => $this->objectInfo['.']['page']->uri,
        )));
      }
    }

    return $page;
  }

  /**
   * Inherits.
   */
  public function addRelationships() {
    module_load_include('inc', 'islandora_paged_content', 'includes/utilities');
    $this->models = "islandora:newspaperPageCModel";
    $relsext = $this->relationships;
    islandora_paged_content_set_relationship($relsext, ISLANDORA_RELS_EXT_URI, 'hasLanguage', $this->preprocessorParameters['language'], TRUE);
    if ($this->preprocessorParameters['generate_ocr']) {
      module_load_include('inc', 'islandora_ocr', 'includes/derivatives');
      islandora_ocr_set_generate_ocr_rels_ext_statement($this, TRUE);
    }
    islandora_paged_content_set_relationship($relsext, FEDORA_RELS_EXT_URI, 'isMemberOf', $this->parentId);
    islandora_paged_content_set_relationship($relsext, ISLANDORA_RELS_EXT_URI, 'isPageOf', $this->parentId);
    islandora_paged_content_set_relationship($relsext, ISLANDORA_RELS_EXT_URI, 'isSequenceNumber', $this->getPageNumber(), TRUE);
    islandora_paged_content_set_relationship($relsext, ISLANDORA_RELS_EXT_URI, 'isPageNumber', $this->getPageNumber(), TRUE);
    parent::inheritXacmlPolicies();
  }

  /**
   * Inherits.
   */
  public function getResources() {
    return array(
      'iarchive_file_path' => array(
        $this->objectInfo['.']['page']->uri,
      ),
    );
  }
}
