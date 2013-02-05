<?php

class UoMNewspaperBatchPreprocessor extends IslandoraBookBatch {
  protected static function get_object_class() {
    return 'UoMNewspaperPaperObject';
  }

  /**
   * Should be able to find all that we need just from the unit.xml files.
   */
  protected get_pattern() {
    return 'unit\.xml';
  }

  protected re_group(&$files, $components, $relative_uri, $file_info) {
    // TODO: Use the three last components (representing the date) as the base name
    $base = implode('-', array_slice($components, -3));
    $files[$base]['.'][$file_info->name] = $file_info;
  }
}

class UoMNewspaperPaperObject extends IslandoraFlatBatchObject {
  public function initial_batch_state() {
    return ISLANDORA_BATCH_STATE__NOT_READY;
  }

  private function get_unit() {
    static $unit = NULL;

    if ($unit === NULL) {
      $unit = simplexml_load_file($this->object_info['.']['unit']->uri);
    }

    return $unit;
  }

  protected function get_children(IslandoraTuque $connection) {
    // TODO: Iterate over page entries in unit.xml, creating children.
    $unit = $this->get_unit();

    $children = array();

    // XXX: drupal_dirname() may not work with ZIP sources...
    $unit_dirname = drupal_dirname($this->object_info['.']['unit']->uri);

    $i = 0;
    foreach ($unit->pages as $page_ref) {
      // Synthesize the "object_info" structure.
      $children[] = new UoMNewspaperPageObject($connection, $this->id, $i++, array(
        '.' => array(
          'page' => $unit_dirname . '/' . $page_ref['href'],
        ),
      ));
    }

    return $children;
  }

  protected function get_mods() {
    // TODO: Transform the unit.xml to MODS (if we don't already have MODS).
    // TODO: Fall-back to parent implementation.
  }

  protected function batch_process() {
    $parent_return = parent::batch_process();

    // TODO: Add the article XML entries onto the newspaper object.

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

  public batch_process() {
    $parent_return = parent::batch_process();

    // Grab the TIFF (as OBJ).
    $this->add_tiff();
    // Grab the PDF for the page.
    $this->add_pdf();
    // Grab the whatever_sm.jpg version (as JPG)
    $this->add_sm_jpg();

    // XXX: Needs to be made to use the shared image derivative module/library
    //   (didn't actually exist at the time of writing).
    // Generate the JP2 and TN.
    $uploaded_file = $this->find_tiff();
    if ($uploaded_file) {
      $jp2 = islandora_large_image_create_JP2_derivative($this, $uploaded_file, $base_name);
      $tn = islandora_large_image_create_TN_derivative($this, $uploaded_file, $base_name);
      file_unmanaged_delete($uploaded_file);
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
    static $path = NULL;

    if ($path === NULL) {
      // Get the URI of the page.xml
      $page_uri = $this->object_info['.']['page']->uri;
      // Break into components.
      $components = explode('/', $page_uri);

      // Add in component for TIFF directory
      array_splice($components, -6, 0, 'TIFFS');
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
    static $path = NULL;

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
    static $path = NULL;

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

  protected function get_title() {
    static $page_number = NULL;
    // Return the page/sequence number.
    if ($page_number === NULL) {
      $p = $this->get_page()->xpath('/ia:page/ia:header-item[@name="page"]');
      if (count($p) > 0) {
        $page_number = $p[0]['value'];
      }
    }

    return $page_number;
  }

  protected function get_mods() {
    // Process the page.xml into MODS (if we don't already have MODS).
    if (!isset($this['MODS'])) {
      $mods = $this->constructDatastream('MODS', 'X');
      $mods->mimetype = 'text/xml';
      $mods->label = 'MODS Record';
      $mods->content = static::run_xsl_transform(array(
        'input' => file_get_contents(),
        'xsl' => drupal_get_path('module', 'uofm_newspaper_batch') . '/transforms/iarchive_to_mods.xsl',
      ));

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
    static $page = NULL;

    if ($page === NULL) {
      $page = simplexml_load_file($this->object_info['.']['page']->uri);
      $page->registerXPathNamespace('ia', 'http://www.iarchives.com/schema/2002/export');
    }

    return $page;
  }
}

