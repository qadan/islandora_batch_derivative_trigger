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
      'aliases' => array('deriv-regen', 'idr'),
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
      'aliases' => array('md-regen', 'imr'),
      'description' => dt('Regenerates metadata for the objects specified'),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
    ),
    'islandora-generate-collection-datastreams-get-metadata-mappings' => array(
      'aliases' => array('md-mappings', 'igmm'),
      'description' => dt('Gets a list of metadata mapping strategies for the given object, content model, or all mappings'),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
      'options' => array(
        'pid' => array(
          'description' => dt('A PID to check mappings for. For a single object, this will check the content models associated with that PID. For a collection, it will check the content models in the collection policy. For a content model, it will check that content model. Without it, all mappings will be returned.'),
          'required' => FALSE,
        ),
      ),
      'examples' => array(
        'List all metadata mappings' => 'drush -u 1 md-mappings',
        'List all metadata mappings for an object' => 'drush -u 1 md-mappings --pid=islandora:sample_object',
        'List all metadata mappings for a content model' => 'drush -u 1 md-mappings --pid=islandora:sampleCModel',
        'List all metadata mappings for the content models in a collection\'s policy' => 'drush -u 1 md-mappings --pid=islandora:sample_collection',
      ),
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
 * Drush command for getting metadata mappings.
 */
function drush_islandora_generate_collection_datastreams_get_metadata_mappings() {
  $pid = drush_get_option('pid');
  if (!is_null($pid)) {
    $object = islandora_object_load($pid);
    if ($object) {
      if (in_array('fedora-system:ContentModel-3.0', $object->models)) {
        $content_models = array($object->id);
      }
      elseif (in_array('islandora:collectionCModel', $object->models)) {
        module_load_include('inc', 'islandora_generate_collection_datastreams', 'includes/utilities');
        $collection_models = islandora_generate_collection_datastreams_get_child_content_models($object);
        $content_models = array();
        foreach ($collection_models as $collection_model) {
          $content_models[] = $collection_model['pid'];
        }
      }
      else {
        $content_models = array_diff($object->models, array('fedora-system:FedoraObject-3.0'));
      }
    }
    else {
      drush_set_error('get_metadata_mappings', dt('Error: could not load object from PID @pid.', array(
        '@pid' => $pid,
      )));
      return;
    }
  }
  else {
    module_load_include('inc', 'islandora', 'includes/utilities');
    $content_models = array_keys(islandora_get_content_models());
  }

  foreach ($content_models as $content_model) {
    $xslts = islandora_generate_collection_datastreams_regenerate_metadata_get_dc_xslts($content_model);
    drush_print(str_repeat('-', strlen($content_model)));
    drush_print($content_model);
    drush_print(str_repeat('-', strlen($content_model)));
    if (empty($xslts)) {
      drush_print('N/A');
    }
    else {
      foreach ($xslts as $xslt) {
        drush_print($xslt);
      }
    }
  }
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
