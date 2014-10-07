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

  /**
   * Inherits.
   */
  protected function getMods() {

    // Patch the MODS if it exists
    if (isset($this['MODS'])) {
      if (isset($this['MODS']->content)) {
        $xml = simplexml_load_string($this['MODS']->content);
        $xml2 = $this->patchMods($xml);
        if (strlen($xml2->asXml()) > 0) {
          $this['MODS']->content($xml2->asXml());
        }
      }
    }

    // Allow fall-back to the parent implementation.
    return parent::getMods();
  }

  /**
   * Modifies the MODS record to include Creative Commons accessConditions,
   * and frequency from the parent newspaper item.
   *
   * @param SimpleXMLElement $xml
   *
   * @return SimpleXMLElement
   */
  protected function patchMods($xml) {
    if (isset($this->preprocessorParameters['mods'])) {
      $parent_xml = simplexml_load_string($this->preprocessorParameters['mods']);
      $parent_xml->registerXPathNamespace('mods', 'http://www.loc.gov/mods/v3');
      $access = $parent_xml->xpath('//mods:accessCondition[@type="Creative Commons License"]');
      $freq = $parent_xml->xpath('//mods:originInfo/mods:frequency[@authority="marcfrequency"]');
      $changes = array_merge($access, $freq);
      
      $target = dom_import_simplexml($xml)->ownerDocument;    
      $xml->registerXPathNamespace('mods', 'http://www.loc.gov/mods/v3');
      foreach ($changes as $ele1) {
        $insert_dom = $target->importNode(dom_import_simplexml($ele1), TRUE);
        $target->documentElement->insertBefore($insert_dom);
      }
    }
    return $xml;
  }
}


