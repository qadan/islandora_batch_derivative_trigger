<?php

/**
 * @file
 * Drush commands to wrap the batch functionality
 */

/**
 * Implements hook_drush_command().
 */
function islandora_batch_derivative_trigger_drush_command() {
  return array(
    'islandora-batch-derivative-trigger-regenerate-derivatives' => array(
      'aliases' => array('deriv-regen', 'idr'),
      'description' => dt('Regenerates derivatives for the objects specified - either all objects in a collection potentially filtered by content model, or specific objects by PID - for all or given filtered DSIDs.'),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
      'options' => array(
        'pids' => array(
          'description' => dt('A comma-separated list of PIDs to regenerate derivatives for. Overrides the collection and content_models options.'),
          'required' => FALSE,
        ),
        'collection' => array(
          'description' => dt('The PID of a collection to target for derivative regeneration. Overridden if a PID list is supplied.'),
          'required' => FALSE,
        ),
        'content_models' => array(
          'description' => dt('An optional comma-separated list of PIDs of content models to filter derivative regeneration for. Overridden if a PID list is supplied.'),
          'required' => FALSE,
        ),
        'dsids' => array(
          'description' => dt('An optional comma-separated list of destination DSIDs to filter derivative regeneration by.'),
          'required' => FALSE,
        ),
        'force' => array(
          'description' => dt('Whether or not to force generation for every derivative, even if it already exists on the object; defaults to "false", and pretty much any value given here will set it as "true".'),
          'required' => FALSE,
        ),
      ),
      'examples' => array(
        'Regenerate all derivatives for specified objects' => 'drush deriv-regen --pids=islandora:1,islandora:2,islandora:3',
        'Regenerate collection derivatives for the specifid content model and DSID' => 'drush dr --collection=sample:collection --content_model=sample:CModel --dsids=DSID,DSID2',
      ),
    ),
    'islandora-batch-derivative-trigger-regenerate-dc-metadata' => array(
      'aliases' => array('dcmd-regen', 'idcmr'),
      'description' => dt('Regenerates DC metadata on specified objects using a given metadata mapping strategy.'),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
      'options' => array(
        'pids' => array(
          'description' => dt('A comma-separated list of PIDs to regenerate DC metadata for.'),
          'required' => FALSE,
        ),
        'collection' => array(
          'description' => dt('The PID of a collection to target for DC metadata regeneration.'),
          'required' => FALSE,
        ),
        'content_models' => array(
          'description' => dt('An optional comma-separated list of content models to filter DC metadata regeneration for.'),
          'required' => FALSE,
        ),
        'mapping' => array(
          'description' => dt('The metadata mapping strategy to use for DC metadata regeneration. If the usage of default DC XSLTs is turned on, these mappings take the form of the ID they carry. Otherwise, mappings take the format "transform.xsl:DSID", where "transform.xsl" is the name of a transformation XSLT defined using the DC transformation XSLT hook, and "DSID" is the name of the destination DSID. Use "drush md-mappings" to get a list of validly-defined metadata mapping strategies; using mapping strategies outside of this valid list can prodice undesired behaviour and is discouraged. Check "drush help md-mappings" for more information.'),
          'required' => TRUE,
        ),
      ),
      'examples' => array(
        'Regenerate DC metadata for the selected objects' => 'drush dcmd-regen --pids=islandora:1,islandora:2 --mapping=mods_to_dc.xslt:MODS',
        'Regenerate DC metadata for all objects of the given content model in a collection' => 'drush dcmd-regen --collection=sample:collection --content_model=sample:CModel --mapping=default_dc_xslt_machine_name',
      ),
    ),
    'islandora-batch-derivative-trigger-get-metadata-mappings' => array(
      'aliases' => array('md-mappings', 'igmm'),
      'description' => dt('Gets a list of metadata mapping strategies for the given object, content model, or all mappings'),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
      'options' => array(
        'pid' => array(
          'description' => dt('A PID to check mappings for. If this PID represents a collection, it will check for mappings associated with the content models of any objects currently in the collection. If it represents a content model, it will check for mappings associated with that content model. If it represents any other object type, it will check for mappings associated with any of the object\'s content models. If this option is not used, all defined mappings will be returned.'),
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
 * Populates a basic $batch_info array based on set drush options.
 */
function islandora_batch_derivative_trigger_get_basic_batch_info() {
  $return = array();
  $collection = drush_get_option('collection');
  $pids = islandora_batch_derivative_trigger_get_option_array_or_null(drush_get_option('pids'));
  $content_models = islandora_batch_derivative_trigger_get_option_array_or_null(drush_get_option('content_models'));
  $return['pids'] = $pids;
  if (!is_null($collection)) {
    $collection = islandora_object_load($collection);
    if (!$collection) {
      drush_set_error('regenerate_derivatives', dt('Error: The collection passed in was invalid.'));
      return FALSE;
    }
    $return['collection'] = $collection;
    if (!is_array($content_models)) {
      module_load_include('inc', 'islandora_batch_derivative_trigger', 'includes/utilities');
      $content_models = array();
      foreach (islandora_batch_derivative_trigger_get_child_content_models($collection) as $content_model) {
        $content_models[] = $content_model['pid'];
      }
    }
    $return['content_models'] = $content_models;
  }
  elseif (is_array($content_models)) {
    $return['collection'] = NULL;
    $return['content_models'] = $content_models;
  }
  elseif (is_null($pids)) {
    drush_set_error('regenerate_derivatives', dt('No collection or PIDs were passed in.'));
    return FALSE;
  }
  else {
    $return['collection'] = NULL;
    $return['content_models'] = NULL;
  }
  return $return;
}

/**
 * Command executor for derivative regeneration.
 */
function drush_islandora_batch_derivative_trigger_regenerate_derivatives() {
  $force = drush_get_option('force');
  $force = is_null($force) ? FALSE : TRUE;
  $batch_info = islandora_batch_derivative_trigger_get_basic_batch_info();
  if (!$batch_info) {
    return;
  }
  module_load_include('inc', 'islandora_batch_derivative_trigger', 'includes/batch');
  $dsids = islandora_batch_derivative_trigger_get_option_array_or_null(drush_get_option('dsids'));
  if (!is_array($dsids) && is_array($batch_info['content_models'])) {
    $dsids = array();
    foreach ($batch_info['content_models'] as $content_model) {
      $dsids += array_keys(islandora_batch_derivative_trigger_regenerate_derivatives_form_get_derivatives_list($content_model, array($batch_info['collection'])));
    }
    $dsids = array_unique($dsids);
  }
  batch_set(islandora_batch_derivative_trigger_regenerate_derivatives_batch($batch_info['pids'], $dsids, $batch_info['collection'], $batch_info['content_models'], $force));
  drush_backend_batch_process();
}

/**
 * Drush command for metadata regeneration.
 */
function drush_islandora_batch_derivative_trigger_regenerate_dc_metadata() {
  $batch_info = islandora_batch_derivative_trigger_get_basic_batch_info();
  if (!$batch_info) {
    return;
  }
  $mapping = drush_get_option('mapping');
  if (is_null($mapping)) {
    drush_set_error('generate_derivatives', dt('No DC metadata mapping was provided.'));
    return;
  }
  module_load_include('inc', 'islandora_batch_derivative_trigger', 'includes/batch');
  $batches = FALSE;
  if (is_array($batch_info['pids'])) {
    batch_set(islandora_batch_derivative_trigger_regenerate_metadata_batch($batch_info['pids'], $mapping, NULL, NULL));
    $batches = TRUE;
  }
  else {
    foreach ($batch_info['content_models'] as $content_model) {
      if (in_array($mapping, array_keys(islandora_batch_derivative_trigger_regenerate_metadata_get_dc_xslts($content_model)))) {
        batch_set(islandora_batch_derivative_trigger_regenerate_metadata_batch(NULL, $mapping, $batch_info['collection'], $content_model));
        $batches = TRUE;
      }
      else {
        drush_set_error('generate_derivatives', dt('The mapping "@mapping" is not valid for use with the @content_model content model. Use "drush md-mappings --pid=@content_model" to get a list of valid mappings for this content model.', array(
          '@mapping' => $mapping,
          '@content_model' => $content_model,
        )));
      }
    }
  }
  if ($batches) {
    drush_backend_batch_process();
  }
}

/**
 * Drush command for getting metadata mappings.
 */
function drush_islandora_batch_derivative_trigger_get_metadata_mappings() {
  $pid = drush_get_option('pid');
  if (!is_null($pid)) {
    $object = islandora_object_load($pid);
    if ($object) {
      if (in_array('fedora-system:ContentModel-3.0', $object->models)) {
        $content_models = array($object->id);
      }
      elseif (in_array('islandora:collectionCModel', $object->models)) {
        module_load_include('inc', 'islandora_batch_derivative_trigger', 'includes/utilities');
        $collection_models = islandora_batch_derivative_trigger_get_child_content_models($object);
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
    $xslts = array_keys(islandora_batch_derivative_trigger_regenerate_metadata_get_dc_xslts($content_model));
    drush_print(str_repeat('-', strlen($content_model)));
    drush_print($content_model);
    drush_print(str_repeat('-', strlen($content_model)));
    if (empty($xslts)) {
      drush_print('N/A');
    }
    else {
      foreach ($xslts as $xslt) {
        drush_print($xslt . "\n");
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
function islandora_batch_derivative_trigger_get_option_array_or_null($option) {
  if (!is_null($option)) {
    $option = explode(',', $option);
  }
  return $option;
}
