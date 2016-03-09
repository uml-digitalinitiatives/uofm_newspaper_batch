<?php

/**
 * @file
 * Classes and functions related to the islandora_batch framework.
 */

/**
* Attempt to reparse a string into a more easily usable format.
*
* @param string $date_string_in
*   A date we will create a new DateTime from.
*
* @return string
*   A string of the format YYYY-MM-DD.
*/
function uofm_newspaper_batch_fix_date($date_string_in, $format=NULL) {
  if (!is_null($format)) {
    $date = DateTime::createFromFormat($format, $date_string_in);
  }
  else {
    $date = new DateTime($date_string_in);
  }
  if ($date) {
    return $date->format('Y-m-d');
  }
  else {
    return $date_string_in;
  }
}

class UoMNewspaperPaperObject extends IslandoraFlatBatchObject {

  protected $patched = FALSE;
  
  /**
   * Inherits.
   */
  protected function getMods() {

    // Patch the MODS if it exists
    if (isset($this['MODS']) && !$this->patched &&
      isset($this->preprocessorParameters['mods_patch'])) {

      $this->patched = TRUE;
      if (isset($this['MODS']->content)) {
        module_load_include('inc', 'manidora', 'includes/XMLPatcher.class');
        $xml = $this['MODS']->content;
        try {
          $new_xml = \umanitoba\manidora\XMLPatcher::patch($xml, $this->preprocessorParameters['mods_patch']);
          if (strlen($new_xml) > 0) {
            $this['MODS']->setContentFromString($new_xml);
          }
        }
        catch (\umanitoba\manidora\XMLPatcherException $e) {
          if ($e->getCode() == 905) {
            // Invalid XML
            drush_log("Unable to patch MODS record. Invalid XML.", "error");
          }
        }
      }
    }

    // Allow fall-back to the parent implementation.
    return parent::getMods();
  }

  /**
   * Inherits.
   *
   * Modified to allow for parameters array of format:
   * $param = array(
   *   namespace => array(
   *      param-name => value,
   *   ),
   * );
   * Removes parameter before adding so last instance wins.
   */
  protected static function runXslTransform($info) {
    $xsl = new DOMDocument();
    $xsl->load($info['xsl']);
    $input = new DOMDocument();
    $input->loadXML($info['input']);
    $processor = new XSLTProcessor();
    $processor->importStylesheet($xsl);
    if (isset($info['php_functions'])) {
      $processor->registerPHPFunctions($info['php_functions']);
    }
    if (isset($info['parameters'])) {
      foreach ($info['parameters'] as $ns => $params) {
        foreach ($params as $n => $v) {
          $processor->removeParameter($ns, $n);
          $processor->setParameter($ns, $n, $v);
        }
      }
    }
    // XXX: Suppressing warnings regarding unregistered prefixes.
    return $processor->transformToXML($input);
  }

}


