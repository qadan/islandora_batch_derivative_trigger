<?php
/**
 * @file
 * Batch functions for regenerating derivatives and metadata.
 */

require_once drupal_get_path('module', 'islandora_basic_collection') . '/includes/batch.inc';

/**
 * A batch 'finished' callback for derivative regeneration that includes logs.
 */
function islandora_generate_collection_datastreams_derivative_regeneration_batch_finished($success, $results, $operations) {
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
 * @param string[] $children
 *   An array of child PIDS to regenerate derivatives for.
 * @param string[] $dsids
 *   An array of DSIDs representing the derivative destination DSIDs that should
 *   be generated.
 * @param AbstractObject $collection
 *   The collection we are generating derivatives for.
 * @param string $content_model
 *   The content model to use in the case of a call to 'regenerate all'.
 *
 * @return array
 *   A Drupal batch array.
 */
function islandora_generate_collection_datastreams_regenerate_derivatives_batch($children, $dsids, AbstractObject $collection, $content_model) {
  $batch = array(
    'finished' => 'islandora_generate_collection_datastreams_derivative_regeneration_batch_finished',
    'progress_message' => t('Time elapsed: @elapsed <br/>Estimated time remaining @estimate.'),
    'error_message' => t('An error has occurred.'),
    'file' => drupal_get_path('module', 'islandora_generate_collection_datastreams') . '/includes/batch.inc',
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
        'islandora_generate_collection_datastreams_regenerate_derivatives_batch_operation',
        array($child, $dsids),
      );
    }
  }
  else {
    $batch += array(
      'title' => 'Regenerating selected derivatives for all children in the collection.',
      'init_message' => 'Preparing to regenerate derivatives for all children in the collection. Be aware that if one or more DSIDs with longer processing times were selected, this may take a long time.',
      'operations' => array(
        array(
          'islandora_generate_collection_datastreams_regenerate_all_derivatives_batch_operation',
          array($dsids, $collection, $content_model),
        ),
      ),
    );
  }
  return $batch;
}

/**
 * Batch operation for the regenerate derivatives batch.
 *
 * @param string $child
 *   The child to perform the regeneration for.
 * @param string[] $dsids
 *   The DSIDs to regenerate on the child.
 * @param array $context
 *   The batch context.
 */
function islandora_generate_collection_datastreams_regenerate_derivatives_batch_operation($child, array $dsids, &$context) {
  module_load_include('inc', 'islandora', 'includes/derivatives');
  if (!isset($context['sandbox']['progress'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['total'] = count($dsids);
  }
  $child = islandora_object_load($child);
  if ($child) {
    foreach ($dsids as $dsid) {
      $context['results'][] = islandora_do_derivatives($child, array(
        'force' => TRUE,
        'destination_dsid' => $dsid,
      ));
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
 * @param array $dsids
 *   The DSIDs to regenerate derivatives for.
 * @param AbstractObject $collection
 *   The collection we are regenerating child derivatives in.
 * @param string $content_model
 *   The content model to regenerate all derivatives for.
 * @param array $context
 *   The batch context.
 */
function islandora_generate_collection_datastreams_regenerate_all_derivatives_batch_operation(array $dsids, AbstractObject $collection, $content_model, &$context) {
  module_load_include('inc', 'islandora', 'includes/derivatives');
  $results = islandora_basic_collection_get_member_objects($collection, $context['sandbox']['page'], 1, 'manage', $content_model);
  $context['sandbox']['page']++;
  if ($results) {
    list ($count, $members) = $results;
    if (!isset($context['sandbox']['current'])) {
      $context['sandbox']['total_count'] = $count;
      $context['sandbox']['current'] = 0;
    }
    foreach ($members as $result) {
      $child = $result['object']['value'];
      if ($child && $child = islandora_object_load($child)) {
        foreach ($dsids as $dsid) {
          $logs = islandora_do_derivatives($child, array(
            'force' => TRUE,
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
        $context['sandbox']['current']++;
      }
    }
  }

  $context['finished'] = $context['sandbox']['current'] / $context['sandbox']['total_count'];
  $context['message'] = t('Regenerated selected derivatives for @current_num @cmodel children in @source.', array(
    '@current_num' => $context['sandbox']['current'],
    '@cmodel' => $content_model,
    '@source' => $collection->label,
  ));
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
function islandora_generate_collection_datastreams_regenerate_metadata_batch($children, $mapping, AbstractObject $collection, $content_model) {
  $batch = array(
    'finished' => 'islandora_generate_collection_datastreams_derivative_regeneration_batch_finished',
    'progress_message' => t('Time elapsed: @elapsed <br/>Estimated time remaining @estimate.'),
    'error_message' => t('An error has occurred.'),
    'file' => drupal_get_path('module', 'islandora_generate_collection_datastreams') . '/includes/batch.inc',
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
        'islandora_generate_collection_datastreams_regenerate_metadata_batch_operation',
        array($child, $mapping),
      );
    }
  }
  else {
    $batch += array(
      'title' => 'Regenerating DC metadata for all children.',
      'init_message' => 'Preparing to regenerate DC metadata for all children.',
      'operations' => array(
        array(
          'islandora_generate_collection_datastreams_regenerate_all_metadata_batch_operation',
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
function islandora_generate_collection_datastreams_regenerate_metadata_batch_operation($child, $mapping, &$context) {
  if (!isset($context['sandbox']['progress'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['total'] = 1;
  }
  $child = islandora_object_load($child);
  if ($child) {
    islandora_generate_collection_datastreams_regenerate_dc_metadata($child, $mapping);
    $context['sandbox']['progress']++;
    $context['message'] = "Regenerated DC metadata for object {$child->id} ({$child->label})";
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
function islandora_generate_collection_datastreams_regenerate_all_metadata_batch_operation($mapping, $collection, $content_model, &$context) {
  $results = islandora_basic_collection_get_member_objects($collection, $context['sandbox']['page'], 10, 'manage', $content_model);
  $context['sandbox']['page']++;
  if ($results) {
    list ($count, $members) = $results;
    if (!isset($context['sandbox']['current'])) {
      $context['sandbox']['total_count'] = $count;
      $context['sandbox']['current'] = 0;
    }
    foreach ($members as $result) {
      $child = $result['object']['value'];
      if ($child && $child = islandora_object_load($child)) {
        islandora_generate_collection_datastreams_regenerate_dc_metadata($child, $mapping);
        $context['sandbox']['current']++;
      }
    }
  }

  $context['finished'] = $context['sandbox']['current'] / $context['sandbox']['total_count'];
  $context['message'] = t('Regenerated DC metadata for @current_num @cmodel children in @source.', array(
    '@current_num' => $context['sandbox']['current'],
    '@cmodel' => $content_model,
    '@source' => $collection->label,
  ));
}

/**
 * Actual DC metadata regeneration functionality.
 *
 * @param AbstractObject $child
 *   The child to regenerate DC metadata for.
 * @param string $mapping
 *   The mapping strategy to use.
 */
function islandora_generate_collection_datastreams_regenerate_dc_metadata(AbstractObject $child, $mapping) {
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
      return FALSE;
    }
  }
  // In the case where they're not.
  else {
    list ($xslt, $source) = explode(":", $mapping);
  }
  $document = new DOMDocument();
  if ($document->loadXML($child[$source]->content)) {
    return xml_form_builder_update_dc_datastream($child, $xslt, $document);
  }
  return FALSE;
}
