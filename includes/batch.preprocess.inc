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

    // TODO: Grab the TIFF (as OBJ).
    // TODO: Grab the PDF for the page.
    // TODO: Grab the whatever_sm.jpg version (as JPG)
    // TODO: Generate the JP2 and TN.

    return ISLANDORA_BATCH_STATE__DONE;
  }

  protected function get_mods() {
    // TODO: Process the page.xml into MODS (if we don't already have MODS).
    

    // Allow fall-back to the parent implementation.
    return parent::get_mods();
  }

  private function get_page() {
    static $page = NULL;

    if ($page === NULL) {
      $page = simplexml_load_file($this->object_info['.']['page']->uri);
    }

    return $page;
  }
}

