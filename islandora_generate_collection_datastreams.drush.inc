<?php

/**
 * @file
 * Drush commands to wrap the batch functionality
 */

/**
 * Implements hook_drush_command().
 */
function islandora_generate_collection_datastreams_drush_command() {
  return array(
    'islandora-generate-collection-datastreams-regenerate-derivatives' => array(
      'aliases' => array('deriv-regen', 'dr'),
      'description' => dt('Regenerates derivatives for the objects specified - either all objects in a collection potentially filtered by content model, or specific objects by PID - for all or given filtered DSIDs.'),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
      'options' => array(
        'pids' => array(
          'description' => dt('A comma-separated list of PIDs to regenerate derivatives for.'),
          'required' => FALSE,
        ),
        'collection' => array(
          'description' => dt('The PID of a collection to target for derivative regeneration.'),
          'required' => FALSE,
        ),
        'content_models' => array(
          'description' => dt('An optional comma-separated list of PIDs of content models to filter derivative regeneration for.'),
          'required' => FALSE,
        ),
        'dsids' => array(
          'description' => dt('An optional comma-separated list of DSIDs to filter derivative regeneration by.'),
          'required' => FALSE,
        ),
      ),
      'examples' => array(
        'Regenerate all derivatives for specified objects' => 'drush deriv-regen --pids=islandora:1,islandora:2,islandora:3',
        'Regenerate collection derivatives for the specifid content model and DSID' => 'drush dr --collection=sample:collection --content_model=sample:CModel --dsids=DSID,DSID2',
      ),
    ),
    'islandora-generate-collection-datastreams-regenerate-metadata' => array(
      'aliases' => array('md-regen', 'mr'),
      'description' => dt('Regenerates metadata for the objects specified'),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
    ),
  );
}

/**
 * Command executor for derivative regeneration.
 */
function drush_islandora_generate_collection_datastreams_regenerate_derivatives() {
  $collection = drush_get_option('collection');
  $dsids = islandora_generate_collection_datastreams_get_option_array_or_null(drush_get_option('dsids'));
  $pids = islandora_generate_collection_datastreams_get_option_array_or_null(drush_get_option('pids'));
  if (!is_null($collection)) {
    $collection = islandora_object_load($collection);
    if (!$collection) {
      drush_set_error('regenerate_derivatives', dt('The collection passed in was invalid.'));
      return;
    }
    $content_models = islandora_generate_collection_datastreams_get_option_array_or_null(drush_get_option('content_models'));
    if (!is_array($content_models)) {
      module_load_include('inc', 'islandora_generate_collection_datastreams', 'includes/batch');
      module_load_include('inc', 'islandora_generate_collection_datastreams', 'includes/utilities');
      $content_models = array();
      foreach (islandora_generate_collection_datastreams_get_child_content_models($collection) as $content_model) {
        $content_models[] = $content_model['pid'];
      }
    }
    foreach ($content_models as $content_model) {
      if (!is_array($dsids)) {
        $dsids = array_keys(islandora_generate_collection_datastreams_regenerate_derivatives_form_get_derivatives_list($content_model));
      }
      batch_set(islandora_generate_collection_datastreams_regenerate_derivatives_batch(NULL, $dsids, $collection, $content_model));
    }
  }
  elseif (is_array($pids)) {
    module_load_include('inc', 'islandora_generate_collection_datastreams', 'includes/batch');
    batch_set(islandora_generate_collection_datastreams_regenerate_derivatives_batch($pids, $dsids, NULL, NULL));
  }

  else {
    drush_set_error('regenerate_derivatives', dt('No collection or PIDs were passed in.'));
  }
  drush_backend_batch_process();
}

/**
 * Drush command for metadata regeneration.
 */
function drush_islandora_generate_collection_datastreams_regenerate_metadata() {
}

/**
 * Explodes an option or returns it as NULL.
 *
 * @param string|null $option
 *   An option to either be converted to an array or just returned as NULL.
 *
 * @return array|null
 *   The exploded option, or NULL if it wasn't a string.
 */
function islandora_generate_collection_datastreams_get_option_array_or_null($option) {
  if (!is_null($option)) {
    $option = explode(',', $option);
  }
  return $option;
}
