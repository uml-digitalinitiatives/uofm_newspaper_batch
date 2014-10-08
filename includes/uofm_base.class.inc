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
function uofm_newspaper_batch_fix_date($date_string_in) {
  $date = DateTime::createFromFormat('d/m/Y', $date_string_in);
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
        module_load_include('inc', 'manidora', 'includes/utilities');
        $xml = $this['MODS']->content;
        $new_xml = manidora_xml_patcher($this->preprocessorParameters['mods_patch'], $xml);
        if (strlen($new_xml) > 0) {
          $this['MODS']->setContentFromString($new_xml);
        }
      }
    }

    // Allow fall-back to the parent implementation.
    return parent::getMods();
  }

}


