<?php
/**
 * @file
 * Batch functions for regenerating derivatives and metadata.
 */

require_once drupal_get_path('module', 'islandora_basic_collection') . '/includes/batch.inc';

/**
 * A batch 'finished' callback for derivative regeneration that includes logs.
 */
function islandora_batch_derivative_trigger_derivative_regeneration_batch_finished($success, $results, $operations) {
  if (!$success) {
    $error_operation = reset($operations);
    $message = t('An error occurred while processing %error_operation with arguments: @arguments', array(
      '%error_operation' => $error_operation[0],
      '@arguments' => print_r($error_operation[1], TRUE)));
    drupal_set_message($message, 'error');
  }
  module_load_include('inc', 'islandora', 'includes/derivatives');
  if (!empty($results['logging'])) {
    islandora_derivative_logging($results['logging']);
  }
}

/**
 * Generates a batch for regenerating derivatives.
 *
 * @param string[]|null $children
 *   An array of child PIDs to regenerate derivatives for, or NULL if using
 *   $collection.
 * @param string[] $dsids
 *   An array of DSIDs representing the derivative destination DSIDs that should
 *   be generated, or NULL to run for all.
 * @param AbstractObject $collection
 *   The collection we are generating derivatives for, or NULL if using
 *   $children.
 * @param array|string $content_models
 *   The content model to use in the case of a call to 'regenerate all'.
 *
 * @return array
 *   A Drupal batch array.
 */
function islandora_batch_derivative_trigger_regenerate_derivatives_batch($children, $dsids, $collection, $content_models, $force = TRUE) {
  $batch = array(
    'finished' => 'islandora_batch_derivative_trigger_derivative_regeneration_batch_finished',
    'progress_message' => t('Time elapsed: @elapsed <br/>Estimated time remaining @estimate.'),
    'error_message' => t('An error has occurred.'),
    'file' => drupal_get_path('module', 'islandora_batch_derivative_trigger') . '/includes/batch.inc',
  );
  if (is_array($children)) {
    $batch += array(
      'title' => format_plural(count($children),
        'Regenerating derivatives for 1 child.',
        'Regenerating derivatives for @count children.'
      ),
      'init_message' => format_plural(count($children),
        'Preparing to regenerate derivatives for 1 child.',
        'Preparing to regenerate derivatives for @count children.'
      ),
    );
    foreach ($children as $child) {
      $batch['operations'][] = array(
        'islandora_batch_derivative_trigger_regenerate_derivatives_batch_operation',
        array($child, $dsids, $force),
      );
    }
  }
  elseif (is_object($collection)) {
    $batch += array(
      'title' => 'Regenerating selected derivatives',
      'init_message' => 'Preparing to regenerate derivatives. Be aware that if one or more DSIDs with longer processing times were selected, this may take a long time.',
      'operations' => array(),
    );
    foreach ((array) $content_models as $content_model) {
      $batch['operations'][] = array(
        'islandora_batch_derivative_trigger_regenerate_all_derivatives_batch_operation',
        array($dsids, $collection, $content_model, $force),
      );
    }
  }
  return $batch;
}

/**
 * Batch operation for the regenerate derivatives batch.
 *
 * @param string $child
 *   The child to perform the regeneration for.
 * @param string[]|null $dsids
 *   The DSIDs to regenerate on the child, or NULL to run all.
 * @param array $context
 *   The batch context.
 */
function islandora_batch_derivative_trigger_regenerate_derivatives_batch_operation($child, $dsids, $force, &$context) {
  module_load_include('inc', 'islandora', 'includes/derivatives');
  if (!isset($context['sandbox']['progress'])) {
    $context['sandbox']['progress'] = 0;
    $child = islandora_object_load($child);
    if (!is_array($dsids)) {
      $appropriate_models = array_diff($child->models, array('fedora-system:FedoraObject-3.0'));
      $context['sandbox']['to_generate'] = array_keys(islandora_batch_derivative_trigger_regenerate_derivatives_form_get_derivatives_list(reset($appropriate_models)));
    }
    else {
      $context['sandbox']['to_generate'] = $dsids;
    }
    $context['sandbox']['total'] = count($context['sandbox']['to_generate']);
    if (!$context['sandbox']['total']) {
      $context['message'] = t('No DSIDs to regenerate derivatives for.');
      $context['finished'] = 1;
      return;
    }
  }
  if ($child) {
    foreach ($context['sandbox']['to_generate'] as $dsid) {
      $logs = islandora_do_derivatives($child, array(
        'force' => $force,
        'destination_dsid' => $dsid,
      ));
      $context['results'][] = $logs;
      foreach ((array) $logs as $log) {
        foreach ($log['messages'] as $message) {
          $return = $message['message'];
          $message_sub = isset($message['message_sub']) ? $message['message_sub'] : array();
          foreach ($message_sub as $sub => $value) {
            $return = str_replace($sub, $value, $return);
          }
          $context['message'] = $return;
        }
      }
      $context['sandbox']['progress']++;
    }
    $context['message'] = "Regenerated selected derivatives for object {$child->id} ({$child->label})";
  }
  else {
    $context['message'] = "Skipping derivative generation for $child: unable to load object.";
  }
  $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['total'];
}

/**
 * Regenerates derivatives for all children in the collection.
 *
 * @param string[]|null $dsids
 *   The DSIDs to regenerate derivatives for.
 * @param AbstractObject $collection
 *   The collection we are regenerating child derivatives in.
 * @param string $content_model
 *   The content model to regenerate all derivatives for.
 * @param array $context
 *   The batch context.
 */
function islandora_batch_derivative_trigger_regenerate_all_derivatives_batch_operation($dsids, $collection, $content_model, $force, &$context) {
  module_load_include('inc', 'islandora', 'includes/derivatives');
  module_load_include('inc', 'islandora_batch_derivative_trigger', 'includes/utilities');
  $sandbox = &$context['sandbox'];
  // Initial prep.
  if (empty($sandbox)) {
    // Get the count.
    $sandbox['total_count'] = islandora_batch_derivative_trigger_get_member_objects($collection, $content_model, NULL, NULL, TRUE);
    // Exit if there weren't any results.
    if (!$sandbox['total_count']) {
      $context['message'] = t('No objects found in collection @collection with content model @model.', array(
        '@collection' => $collection,
        '@model' => $content_model,
      ));
      $context['finished'] = 1;
      return;
    }
    // Keep a current for display purposes.
    $sandbox['current'] = 0;
    // Hang onto some slice information.
    $sandbox['slice'] = NULL;
  }

  $results = islandora_batch_derivative_trigger_get_member_objects($collection, $content_model, $sandbox['slice'], 1);
  // If nothing came back, we're done.
  if (empty($results)) {
    $context['finished'] = 1;
  }
  foreach ($results as $result) {
    $sandbox['current']++;
    $sandbox['slice'] = array(
      'date' => $result['created']['value'],
      'pid' => $result['object']['value'],
    );
    $child = islandora_object_load($result['object']['value']);
    $source_info = array(
      '@model' => $content_model,
      '@collection' => $collection,
    );
    $source_string = is_null($collection) ? t('type: @model', $source_info) : t('type: @model, collection: @collection', $source_info);
    if ($child) {
      if (!is_array($dsids)) {
        $appropriate_models = array_diff($child->models, array('fedora-system:FedoraObject-3.0'));
        $to_generate = array_keys(islandora_batch_derivative_trigger_regenerate_derivatives_form_get_derivatives_list(reset($appropriate_models)));
      }
      else {
        $to_generate = $dsids;
      }
      foreach ($to_generate as $dsid) {
        $logs = islandora_do_derivatives($child, array(
          'force' => $force,
          'destination_dsid' => $dsid,
        ));
        $context['results'][] = $logs;
        foreach ((array) $logs as $log) {
          foreach ($log['messages'] as $message) {
            $return = $message['message'];
            $message_sub = isset($message['message_sub']) ? $message['message_sub'] : array();
            foreach ($message_sub as $sub => $value) {
              $return = str_replace($sub, $value, $return);
            }
            $context['message'] = $return;
          }
        }
      }
      $context['message'] = t('@current_num/@total: Regenerated selected derivatives for @pid (@source).', array(
        '@current_num' => $sandbox['current'],
        '@total' => $sandbox['total_count'],
        '@source' => $source_string,
        '@pid' => $child->id,
      ));
    }
    else {
      $context['message'] = t('@current_num/@total: Failed to load child @pid (@source).', array(
        '@current_num' => $sandbox['current'],
        '@total' => $sandbox['total_count'],
        '@pid' => $result['object']['value'],
        '@source' => $source_string,
      ));
    }
  }

  $context['finished'] = $sandbox['current'] / $sandbox['total_count'];
}

/**
 * Generates a batch for regenerating DC metadata.
 *
 * @param string[] $children
 *   An array of child PIDS to regenerate DC metadata for.
 * @param string[] $mapping
 *   The metadata mapping strategy to use when regenerating derivatives.
 * @param AbstractObject $collection
 *   The collection to perform batch operations on.
 * @param string $content_model
 *   The content model to regenerate for if regenerating all metadata.
 *
 * @return array
 *   A Drupal batch array.
 */
function islandora_batch_derivative_trigger_regenerate_metadata_batch($children, $mapping, AbstractObject $collection, $content_model) {
  $batch = array(
    'finished' => 'islandora_batch_derivative_trigger_derivative_regeneration_batch_finished',
    'progress_message' => t('Time elapsed: @elapsed <br/>Estimated time remaining @estimate.'),
    'error_message' => t('An error has occurred.'),
    'file' => drupal_get_path('module', 'islandora_batch_derivative_trigger') . '/includes/batch.inc',
  );
  if (is_array($children)) {
    $batch += array(
      'title' => format_plural(count($children),
        'Regenerating DC metadata for 1 child.',
        'Regenerating DC metadata for @count children.'
      ),
      'init_message' => format_plural(count($children),
        'Preparing to regenerate DC metadata for 1 child.',
        'Preparing to regenerate DC metadata for @count children.'
      ),
    );
    foreach ($children as $child) {
      $batch['operations'][] = array(
        'islandora_batch_derivative_trigger_regenerate_metadata_batch_operation',
        array($child, $mapping),
      );
    }
  }
  else {
    $batch += array(
      'title' => 'Regenerating DC metadata for all children.',
      'init_message' => 'Preparing to regenerate DC metadata for all children. Bear in mind that the total count may change if the collection is actively being modified.',
      'operations' => array(
        array(
          'islandora_batch_derivative_trigger_regenerate_all_metadata_batch_operation',
          array($mapping, $collection, $content_model),
        ),
      ),
    );
  }
  return $batch;
}

/**
 * Batch operation for the regenerate metadata batch.
 *
 * @param string $child
 *   The child to perform the regeneration for.
 * @param string $mapping
 *   The mapping strategy to use.
 * @param array $context
 *   The batch context.
 */
function islandora_batch_derivative_trigger_regenerate_metadata_batch_operation($child, $mapping, &$context) {
  if (!isset($context['sandbox']['progress'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['total'] = 1;
  }
  $child = islandora_object_load($child);
  if ($child) {
    $result = islandora_batch_derivative_trigger_regenerate_dc_metadata($child, $mapping);
    $context['sandbox']['progress']++;
    $context['message'] = is_string($result) ? "Error: $result" : "Regenerated DC metadata for object {$child->id} ({$child->label})";
  }
  else {
    $context['message'] = "Skipping DC metadata regeneration for $child: unable to load object.";
  }
  $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['total'];
}

/**
 * Batch operation for regenerating all collection metadata in batch.
 *
 * @param string $mapping
 *   The mapping strategy to use.
 * @param AbstractObject $collection
 *   The collection we are regenerating all metadata for.
 * @param string $content_model
 *   The content model to regenerate all metadata for.
 * @param array $context
 *   The batch context.
 */
function islandora_batch_derivative_trigger_regenerate_all_metadata_batch_operation($mapping, $collection, $content_model, &$context) {
  $sandbox = &$context['sandbox'];
  module_load_include('inc', 'islandora_batch_derivative_trigger', 'includes/utilities');
  if (empty($sandbox)) {
    $sandbox['slice'] = NULL;
    // We don't really use this, it's mostly just for interface reporting.
    $sandbox['current'] = 0;
  }
  // The 'total' represents a potentially moving target; recalculating and
  // constantly updating our slice keeps things accurate.
  $sandbox['total'] = islandora_batch_derivative_trigger_get_member_objects($collection, $content_model, NULL, NULL, TRUE);
  // Bail if the collection is empty.
  if ($sandbox['total'] == 0) {
    $context['message'] = t('No items to process in collection @collection (@id) for content model @cmodel.', array(
      '@collection' => $collection->label,
      '@id' => $collection->id,
      '@cmodel' => $content_model,
    ));
    $context['finished'] = 1;
    return;
  }

  $results = islandora_batch_derivative_trigger_get_member_objects($collection, $content_model, $sandbox['slice'], 10);
  if (!empty($results)) {
    // Iterate through the results.
    foreach ($results as $member) {
      $child = islandora_object_load($member['object']['value']);
      $sandbox['slice'] = array(
        'date' => $member['created']['value'],
        'pid' => $member['object']['value'],
      );
      $sandbox['current']++;
      $subs = array(
        '@current_num' => $sandbox['current'],
        '@total' => $sandbox['total'],
        '@cmodel' => $content_model,
        '@source' => $collection->label,
        '@pid' => $member['object']['value'],
      );
      if ($child) {
        $result = islandora_batch_derivative_trigger_regenerate_dc_metadata($child, $mapping);
        if (is_string($result)) {
          $subs['@result'] = $result;
          $context['message'] = t("@current_num/@total: Error: @result", $subs);
        }
        else {
          $context['message'] = t('@current_num/@total: Regenerated DC metadata for @pid (type: @cmodel, collection: @source).', $subs);
        }
      }
      else {
        $context['message'] = t('@current_num/@total: Error: failed to load @pid.');
      }
    }
    $context['finished'] = $sandbox['current'] / $sandbox['total'];
  }
  else {
    // Prevent iterating through another set.
    $context['finished'] = 1;
  }

}

/**
 * Actual DC metadata regeneration functionality.
 *
 * @param AbstractObject $child
 *   The child to regenerate DC metadata for.
 * @param string $mapping
 *   The mapping strategy to use.
 */
function islandora_batch_derivative_trigger_regenerate_dc_metadata(AbstractObject $child, $mapping) {
  if (variable_get('xml_form_builder_use_default_dc_xslts', FALSE)) {
    // In the case where default XSLTs are being used.
    module_load_include('inc', 'xml_form_builder', 'includes/db');
    $xslt_id = db_select('xml_form_builder_default_xslts', 'xd')
      ->fields('xd', array('xslt_id'))
      ->condition('id', $mapping, '=')
      ->execute()
      ->fetchField();
    $transform = db_select('xml_form_builder_xslts', 'xs')
      ->fields('xs', array('xslt'))
      ->condition('xslt_id', $xslt_id, '=')
      ->execute()
      ->fetchField();
    $source = db_select('xml_form_builder_default_xslts', 'xd')
      ->fields('xd', array('dsid'))
      ->condition('xslt_id', $xslt_id, '=')
      ->execute()
      ->fetchField();
    $xslt = new DOMDocument();
    if (!$xslt->loadXML($transform)) {
      return t("The transform being used is not valid XML");
    }
  }
  // In the case where they're not.
  else {
    list ($xslt, $source) = explode(":", $mapping);
  }
  $document = new DOMDocument();
  if (!isset($child[$source])) {
    return t("The object @child is missing the @source datastream.", array(
      '@child' => $child->id,
      '@source' => $source,
    ));
  }
  if ($document->loadXML($child[$source]->content)) {
    return xml_form_builder_update_dc_datastream($child, $xslt, $document);
  }
  return t("The XML found on the @source datastream of @child could not be parsed.", array(
    '@child' => $child->id,
    '@source' => $source,
  ));
}
