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
    'description' => 'Preprocesses newspapers from IArchive folder structure ' .
      'into database entries.',
    'drupal dependencies' => array('uofm_newspaper_batch'),
    'options' => array(
      'target' => array(
        'description' => 'The target to directory to scan.',
        'required' => TRUE,
      ),
      'content_models' => array(
        'description' => 'A comma-separated list of content models to assign ' .
          'to the objects. Only applies to the "book" level object.',
        'value' => 'optional',
      ),
      'parent' => array(
        'description' => 'The collection to which the generated items should ' .
          'be added.  Only applies to the "newspaper issue" level object.',
        'value' => 'optional'
      ),
      'parent_relationship_uri' => array(
        'description' => 'The namespace URI of the relationship to the parent.' .
          ' Defaults to "info:fedora/fedora-system:def/relations-external#".',
        'value' => 'optional',
      ),
      'parent_relationship_pred' => array(
        'description' => 'The predicate of the relationship to the parent. ' .
          'Defaults to "isIssueOf".',
        'value' => 'optional',
      ),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );

  return $items;
}

function drush_uofm_newspaper_batch_preprocess() {
    // Build one of our preprocessors.
  $connection = islandora_get_tuque_connection();
  $parameters = array(
    'type' => 'directory',
    'target' => drush_get_option('target'),
    'parent' => drush_get_option('parent', 'islandora:newspaperCollection'),
    'parent_relationship_uri' => drush_get_option('parent_relationship_uri', 'http://islandora.ca/ontology/relsext#'),
    'parent_relationship_pred' => drush_get_option('parent_relationship_pred', 'isIssueOf'),
    'wait_for_metadata' => FALSE,
    'directory_dedup' => FALSE,
  );

  if ($content_models = drush_get_option('content_models', FALSE)) {
    $parameters['content_models'] = explode(',', $content_models);
  }
  else {
    $parameters['content_models'] = array('islandora:newspaperIssueCModel');
  }

  $preprocessor = new UoMNewspaperBatchPreprocessor($connection, $parameters);

  // Pass the preprocessor off to run.
  $preprocessed = islandora_batch_handle_preprocessor($preprocessor);
}
