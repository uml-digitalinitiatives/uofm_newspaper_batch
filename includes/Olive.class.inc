<?php

/**
 * @file
 * Classes and functions related to the islandora_batch framework.
 */

class UoMOliveNewspaperBatchPreprocessor extends IslandoraBookBatch {
  /**
   * Inherits.
   */
  protected static function getObjectClass() {
    return 'UoMOliveNewspaperPaperObject';
  }

  /**
   * Should be able to find all that we need just from the unit.xml files.
   */
  protected static function getPattern() {
    return '/TOC\.xml/';
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

class UoMOliveNewspaperPaperObject extends UoMNewspaperPaperObject {
  /**
   * Inherits.
   */
  public function initialBatchState() {
    return ISLANDORA_BATCH_STATE__NOT_READY;
  }

  /**
   * Get the table of contents XML, parsed with simple XML.
   */
  protected function getToc() {
    $toc = NULL;

    if ($toc === NULL) {
      $toc = simplexml_load_file($this->objectInfo['.']['TOC']->uri);
    }

    return $toc;
  }

  /**
   * Inherits.
   */
  public function getChildren(IslandoraTuque $connection) {
    // Iterate over page entries in unit.xml, creating children.
    $toc = $this->getToc();

    $children = array();

    // XXX: drupal_dirname() may not work with ZIP sources...
    $toc_dirname = drupal_dirname($this->objectInfo['.']['TOC']->uri);

    $i = 0;
    foreach ($toc->xpath('//Page') as $page_ref) {
      
      // Synthesize the "object_info" structure.
      $page_obj = new stdClass();
      $page_obj->uri = $toc_dirname . $page_ref['HREF'] . ".xml";
      $page_obj->filename = pathinfo($page_obj->uri, PATHINFO_BASENAME);
      $page_obj->name = pathinfo($page_obj->uri, PATHINFO_FILENAME);

      $page_object_info = array(
        '.' => array(
          'page' => $page_obj,
        ),
      );
      $children[] = new UoMOliveNewspaperPageObject($connection, $this->id, $i++,
        $page_object_info, $this->preprocessorParameters);
    }

    return $children;
  }

  /**
   * Inherits.
   */
  protected function getMods() {
    // Process the TOC.xml into MODS (if we don't already have MODS).
    if (!isset($this['MODS'])) {
      $mods = $this->constructDatastream('MODS', 'M');
      $mods->mimetype = 'text/xml';
      $mods->label = 'MODS Record';

      $mods->content = static::runXslTransform(array(
        'input' => file_get_contents($this->objectInfo['.']['TOC']->uri),
        'xsl' => drupal_get_path('module', 'uofm_newspaper_batch') . '/transforms/olive_to_mods.xsl',
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
      'olive_file_path' => array(
        $this->objectInfo['.']['TOC']->uri,
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

    return ISLANDORA_BATCH_STATE__DONE;
  }

}

class UoMOliveNewspaperPageObject extends UoMNewspaperPaperObject {
  protected $parentId;
  protected $tiffPath = NULL;
  protected $date;
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
    // Grab the whatever_sm.jpg version (as JPG).
    // Small JPGs no longer exist by default.
    //$this->addSmJpg();

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
      $tiff_path = $this->findTiff();
      if ($tiff_path) {
        // Add the tiff (if it isn't already on the object).
        $this->tiffPath = $tiff_path;
        $obj = $this->constructDatastream('OBJ', 'M');
        $obj->label = 'Newspaper page';
        $obj->mimetype = 'image/tiff';
        $obj->setContentFromFile($this->tiffPath, FALSE);
        $this->ingestDatastream($obj);
        return $this['OBJ'];
      }
    }
  }


  /**
   * Build the path to where the TIFF is (should be) located.
   */
  protected function findTiff() {
    if ($this->tiffPath === NULL) {
      $xml = simplexml_load_file($this->objectInfo['.']['page']->uri);
      $tiff_name = NULL;
      $res = $xml->xpath('/XMD-PAGE/Meta');
      if (count($res) > 0) {
        foreach ($res[0]->attributes() as $name => $val) {
          if ((string) $name == "RELEASE_NO") {
            $tiff_name = $val;
            break;

          }
        }
      }
      if (empty($tiff_name)) {
        throw new Exception("Could not locate Tiff filename in file @f",
          array("@f" => $this->objectInfo['.']['page']->uri));
      }
      // Because all filenames are the same we have to switch in the
      // correct page number and the case is wrong 
      $new_file = str_pad($this->getPageNumber(), 3, '0', STR_PAD_LEFT);
      $tiff_name = str_replace('001-SINGLE', $new_file . '-Single', $tiff_name);
      
      // Get the URI of the page.xml.
      $page_uri = $this->objectInfo['.']['page']->uri;
      // Break into components.
      $components = explode('/', $page_uri);

      // Add in component for TIFF directory.
      // Below was only needed for the ones I had sorted.
      // array_splice($components, -5, 0, 'TIFFS');
      // Change "pages" to "Archival_Images".
      array_splice($components, -4, 4, 'Archival_images');

      // Change the file extension.
      $components[] = $tiff_name . '.tif';

      $this->tiffPath = implode('/', $components);

    }
    if (file_exists($this->tiffPath)) {
      return $this->tiffPath;
    }
    else {
      return FALSE;
    }
  }

  /**
   * Add the pdf.
   *
   * @return AbstractDatastream
   *   The datastream which has been added to this object.
   */
  protected function addPdf() {
    if (!isset($this['PDF'])) {
      $pdf_path = $this->findPdf();
      if ($pdf_path) {
        // Add the pdf (if it isn't already on the object).
        $obj = $this->constructDatastream('PDF', 'M');
        $obj->label = 'Newspaper page PDF';
        $obj->mimetype = 'application/pdf';
        $obj->setContentFromFile($pdf_path, FALSE);
        $this->ingestDatastream($obj);
        return $this['PDF'];
      }
    }

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
    if (file_exists($path)) {
      return $path;
    }
    else {
      return FALSE;
    }
  }

  /**
   * Add the sm_jpg.
   *
   * @return AbstractDatastream
   *   The datastream which has been added to this object.
   */
  protected function addSmJpg() {
    if (!isset($this['JPG'])) {
      $sm_jpg_path = $this->findSmJpg();
      if ($sm_jpg_path) {
        // Add the sm_jpg (if it isn't already on the object).
        $obj = $this->constructDatastream('PNG', 'M');
        $obj->label = 'Newspaper page';
        $obj->mimetype = 'image/png';
        $obj->setContentFromFile($sm_jpg_path, FALSE);
        $this->ingestDatastream($obj);
        return $this['JPG'];
      }
    }
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
      array_splice($components, -1, 1, 'Img');

      // Change the file extension.
      $xml = array_pop($components);
      $components[] = pathinfo($xml, PATHINFO_FILENAME) . '.png';

      $path = implode('/', $components);
    }

    if (file_exists($path)) {
      return $path;
    }
    else {
      return FALSE;
    }
  }

  /**
   * Get the page number of this page from the page XML.
   */
  protected function getPageNumber() {
    $page_number = NULL;
    // Return the page/sequence number.
    if ($page_number === NULL) {
      $p = $this->getPage()->xpath('/XMD-PAGE/Meta');
      if (count($p) > 0) {
        foreach ($p[0]->attributes() as $k => $v) {
          if ($k == 'PAGE_NO') {
            $page_number = $v;
            break;
            
          }
        }
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
        'xsl' => drupal_get_path('module', 'uofm_newspaper_batch') . '/transforms/olive_to_hocr.xsl',
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
        'xsl' => drupal_get_path('module', 'uofm_newspaper_batch') . '/transforms/olive_to_hocr.xsl',
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
        'xsl' => drupal_get_path('module', 'uofm_newspaper_batch') . '/transforms/olive_to_mods.xsl',
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
   * Get the page XML, parsed with SimpleXML.
   */
  protected function getPage() {
    $page = NULL;

    if ($page === NULL) {
      $page = simplexml_load_file($this->objectInfo['.']['page']->uri);
    }

    return $page;
  }

  /**
   * Inherits.
   */
  public function addRelationships() {
    $this->models = "islandora:newspaperPageCModel";
    // @todo Add hasLanguage?
    $this->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOf', $this->parentId);
    $this->relationships->add(ISLANDORA_RELS_EXT_URI, 'isPageOf', $this->parentId);
    $this->relationships->add(ISLANDORA_RELS_EXT_URI, 'isSequenceNumber', $this->getPageNumber(), TRUE);
    $this->relationships->add(ISLANDORA_RELS_EXT_URI, 'isPageNumber', $this->getPageNumber(), TRUE);
    parent::inheritXacmlPolicies();
  }

  /**
   * Inherits.
   */
  public function getResources() {
    return array(
      'olive_file_path' => array(
        $this->objectInfo['.']['page']->uri,
      ),
    );
  }
}
