<?php

/**
 * @file
 * Implementation of Drush hooks.
 */

/**
 * Implements hook_drush_command().
 */
function uofm_newspaper_batch_drush_command() {
  $items = array();

  $items['uofm_newspaper_batch_preprocess'] = array(
    'aliases' => array('uom_npp'),
    'description' => 'Preprocesses newspapers from IArchive folder structure into database entries.',
    'drupal dependencies' => array('uofm_newspaper_batch'),
    'options' => array(
      'xml_type' => array(
        'description' => 'The version of XML used by the ingested newspaper. Currently only "iarchives" and "olive" are supported.',
        'required' => TRUE,
      ),
      'namespace' => array(
        'description' => 'The namespace into which to ingest.',
        'value' => 'optional',
      ),
      'content_models' => array(
        'description' => 'A comma-separated list of content models to assign to the objects. Only applies to the "book" level object.',
        'value' => 'optional',
      ),
      'parent' => array(
        'description' => 'The collection to which the generated items should be added.  Only applies to the "newspaper issue" level object.',
        'value' => 'optional',
      ),
      'do_not_generate_ocr' => array(
        'description' => 'Skip generate HOCR/OCR for pages.',
        'value' => 'optional',
      ),
      'language' => array(
        'description' => 'Language to use for tesseract when generating OCR and HOCR. Defaults to eng.',
        'value' => 'optional',
      ),
      'parent_relationship_uri' => array(
        'description' => 'The namespace URI of the relationship to the parent. Defaults to "info:fedora/fedora-system:def/relations-external#".',
        'value' => 'optional',
      ),
      'parent_relationship_pred' => array(
        'description' => 'The predicate of the relationship to the parent. Defaults to "isMemberOf".',
        'value' => 'optional',
      ),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );
  // XXX: The target parameter became reserved in Drush 7 and above, for
  // backwards compatibility both will be supported. Not using
  // strict-option-handling (http://www.drush.org/en/master/strict-options) as
  // it requires manual argument parsing.
  if (DRUSH_VERSION >= 7) {
    $items['uofm_newspaper_batch_preprocess']['options']['scan_target'] = array(
      'description' => 'The target to directory to scan.',
      'required' => TRUE,
    );
  }
  else {
    $items['uofm_newspaper_batch_preprocess']['options']['target'] = array(
      'description' => 'The target to directory to scan.',
      'required' => TRUE,
    );
  }
  $items['uofm_newspaper_batch_sequence'] = array(
    'aliases' => array('uofm_np_seq'),
    'description' => 'Fixes isSequenceNumber elements by re-adding them to all newspaperIssue elements of the specified newspaper',
    'drupal dependencies' => array('uofm_newspaper_batch'),
    'options' => array(
      'parent' => array(
        'description' => 'PID of the newspaper object, all issues will be re-sequenced.',
        'required' => TRUE,
      ),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );

  return $items;
}

/**
 * Drush command callback; perform the newspaper preprocessing.
 */
function drush_uofm_newspaper_batch_preprocess() {
  // Build one of our preprocessors.
  $connection = islandora_get_tuque_connection();
  if (!module_exists('islandora_ocr')) {
    $generate_ocr = FALSE;
  }
  else {
    $language = drush_get_option('language', 'eng');
    module_load_include('inc', 'islandora_ocr', 'includes/utilities');
    $langs = islandora_ocr_get_tesseract_installed_languages();
    if (!in_array($language, $langs)) {
      return drush_set_error(t('!lang is not an installed language in tesseract', array('!lang' => $language)));
    }
    $generate_ocr = (bool) (is_null(drush_get_option('do_not_generate_ocr', NULL)));
  }

  $parameters = array(
    'type' => 'directory',
    'target' => DRUSH_VERSION >= 7 ? drush_get_option('scan_target') : drush_get_option('target'),
    'parent' => drush_get_option('parent', 'islandora:newspaperCollection'),
    'parent_relationship_uri' => drush_get_option('parent_relationship_uri', 'info:fedora/fedora-system:def/relations-external#'),
    'parent_relationship_pred' => drush_get_option('parent_relationship_pred', 'isMemberOf'),
    'generate_ocr' => $generate_ocr,
    'language' => $language,
    'wait_for_metadata' => FALSE,
    'directory_dedup' => FALSE,
    'namespace' => drush_get_option('namespace', NULL),
  );

  if ($content_models = drush_get_option('content_models', FALSE)) {
    $parameters['content_models'] = explode(',', $content_models);
  }
  else {
    $parameters['content_models'] = array('islandora:newspaperIssueCModel');
  }

  $object = islandora_object_load($parameters['parent']);
  if ($object && isset($object['MODS'])) {
    $xml = simplexml_load_string($object['MODS']->content);
    $xml->registerXPathNamespace('mods', 'http://www.loc.gov/mods/v3');
    $freq = $xml->xpath('/mods:mods/mods:originInfo/mods:frequency');
    $access = $xml->xpath('/mods:mods/mods:accessCondition[@type="Creative Commons License"]');
    $change = array(
      'ns' => array(
        'mods' => 'http://www.loc.gov/mods/v3',
      ),
      'changes' => array(),
    );
    foreach ($freq as $f) {
      $change['changes'][] = array('type' => 'remove', 'old' => '/mods:mods/mods:originInfo/mods:frequency');
      $change['changes'][] = array('type' => 'add', 'parent' => '/mods:mods/mods:originInfo', 'new' => $f->asXML());
    }
    foreach ($access as $a) {
      $change['changes'][] = array('type' => 'remove', 'old' => '/mods:mods/mods:accessCondition[@type="Creative Commons License"]');
      $change['changes'][] = array('type' => 'add', 'parent' => '/mods:mods', 'new' => $a->asXML());
    }
    if (count($change['changes']) > 0) {
      $parameters['mods_patch'] = $change;
    }
  }

  $xml_type = drush_get_option('xml_type');
  switch (strtolower($xml_type)) {
    case 'iarchives':
      $preprocessor = new UoMIarchivesNewspaperBatchPreprocessor($connection, $parameters);
      break;

    case 'olive':
      $preprocessor = new UoMOliveNewspaperBatchPreprocessor($connection, $parameters);
      break;

    default:
      return drush_set_error(t('"@type" is not a valid xml type. Please choose between "iarchives" and "olive".', array('@type' => $xml_type)));
  }

  // Pass the preprocessor off to run.
  $preprocessed = islandora_batch_handle_preprocessor($preprocessor);
  drush_log(t("New set ID (@s)", array('@s' => $preprocessor->getSetId())), "ok");
}

/**
 * Fixes or adds sequence numbers to newspaper issues.
 */
function drush_uofm_newspaper_batch_sequence() {
  $parent = drush_get_option('parent');
  $connection = islandora_get_tuque_connection();
  $object = islandora_object_load($parent);
  $models = $object->models;
  if (!in_array('islandora:newspaperCModel', $models)) {
    return drush_set_error(t('Selected parent is not of a type islandora:newspaperCModel, exiting...'));
  }
  $query = <<<EOQ
PREFIX islandora-rels-ext: <http://islandora.ca/ontology/relsext#>
SELECT ?obj ?issued FROM <#ri>
WHERE {
  ?obj <fedora-rels-ext:isMemberOf> <info:fedora/{$parent}> ;
  <fedora-model:hasModel> <info:fedora/islandora:newspaperIssueCModel> ;
  islandora-rels-ext:dateIssued ?issued .
}
ORDER BY ?issued
EOQ;

  $results = $object->repository->ri->sparqlQuery($query);
  $counter = 1;
  $operations = array();
  foreach ($results as $result) {
    $operations[] = array('_uofm_newspaper_batch_fix_sequence',
      array($result['object']['value'], $counter),
    );
    $counter += 1;
  }

  $batch = array(
    'operations' => $operations,
    'finished' => '_uofm_newspaper_batch_fix_sequence_finished',
    'title' => t('Recreating Sequence Numbers for Newspaper Issues'),
    'init_message' => t('Batch is starting.'),
    'progress_message' => t('Processed @current out of @total.'),
    'error_message' => t('Batch has encountered an error.'),
    'file' => drupal_get_path('module', 'uofm_newspaper_batch') . '/uofm_newspaper_batch.drush.inc',
  );
  batch_set($batch);
  $batch =& batch_get();
  $batch['progressive'] = FALSE;
  drush_backend_batch_process();
}

/**
 * We want the sequence of newspapers to mimic date order.
 */
function _uofm_newspaper_batch_fix_sequence($pid, $count, &$context) {
  $context['message'] = t("Fixing issue #@c", array('@c' => $count));
  $object = islandora_object_load($pid);
  if ($object) {
    $rels = $object->relationships->get(ISLANDORA_RELS_EXT_URI, "isSequenceNumber");
    if (count($rels) > 0) {
      $object->relationships->remove(ISLANDORA_RELS_EXT_URI, "isSequenceNumber");
    }
    $object->relationships->add(ISLANDORA_RELS_EXT_URI, "isSequenceNumber", (string) $count, TRUE);
    $context['results'][] = $pid;
  }
}

/**
 * Fix the sequence numbers for the issues of a particular newspaper.
 */
function _uofm_newspaper_batch_fix_sequence_finished($success, $results, $operations) {
  if ($success) {
    $message = format_plural(count($results), 'One issue processed.', '@count issues processed.');
  }
  else {
    $message = t('Finished with an error.');
  }
  drupal_set_message($message);
}
