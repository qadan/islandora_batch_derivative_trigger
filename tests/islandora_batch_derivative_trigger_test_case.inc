<?php

/**
 * @file
 * Batch derivative trigger test case.
 */

class IslandoraBatchDerivativeTriggerTestCase extends IslandoraCollectionWebTestCase {

  /**
   * Sets up the tests.
   *
   * @see IslandoraWebTestCase::setUp()
   */
  public function setUp() {
    parent::setUp(array(
      'islandora',
      'batch_derivative_test',
      'islandora_batch_derivative_trigger',
    ));
    $this->testCollection = 'test:' . $this->randomName();
    $user = $this->drupalCreateUser(array_keys(module_invoke_all('permission')));
    $this->drupalLogin($user);
    $this->createTestCollection('Test Collection', array('islandora:batch_testCM'), $this->testCollection);
  }

  /**
   * Test tearDown functionality.
   *
   * @see IslandoraWebTestCase::tearDown()
   */
  public function tearDown() {
    $this->deleteTestCollection($this->testCollection);
    parent::tearDown();
  }

  /**
   * Ingests an object using the test module.
   *
   * @param bool $mods
   *   Whether or not to add a MODS datastream.
   * @param bool $obj
   *   Whether or not to add an OBJ datastream.
   * @param bool $derive
   *   Whether or not to add a DERIVED datastream if we were asked.
   */
  protected function ingestBatchDerivativeTestObject($mods, $obj, $derive = FALSE) {
    // Base properties.
    $properties = array(
      'pid' => 'test',
      'label' => 'Test Object',
      'models' => array('islandora:batch_testCM'),
      'owner' => $this->loggedInUser->name,
      'parent' => $this->testCollection,
    );
    $datastreams = array();
    // MODS datastream.
    if ($mods) {
      $datastreams[] = array(
        'dsid' => 'MODS',
        'path' => drupal_get_path('module', 'batch_derivative_test') . "/fixtures/batch_derivative_test_mods.xml",
        'control_group' => 'M',
        'mimetype' => 'text/xml',
      );
    }
    // OBJ datastream.
    if ($obj) {
      $datastreams[] = array(
        'dsid' => 'OBJ',
        'string' => 'OBJ',
        'control_group' => 'M',
        'mimetype' => 'text/plain',
      );
    }

    $object = $this->ingestConstructedObject($properties, $datastreams);
    if ($derive) {
      batch_derivative_test_derive_derived_datastream($object, FALSE);
    }

    // Return the object.
    return $object;
  }

  /**
   * Fills out the batch derivative regeneration form and submits it.
   *
   * @param string $collection_pid
   *   The PID of the collection to trigger derivatives for.
   * @param string $content_model
   *   The content model to trigger for.
   * @param string[] $dsids
   *   An array of DSID strings to regenerate.
   * @param bool|string[] $pids
   *   An array of PID strings to regenerate for, or TRUE to regenerate all.
   * @param bool $force
   *   Whether or not to force triggering.
   */
  protected function triggerBatchDerivatives($collection_pid, $content_model, $dsids, $pids = TRUE, $force = FALSE) {
    // Regenerate the page based on content model.
    $path = "islandora/object/$collection_pid/manage/collection";
    $edit = array(
      'content_model' => $content_model,
    );
    $this->drupalPostAJAX($path, $edit, 'content_model', NULL, array(), array(), 'islandora-batch-derivative-trigger-regenerate-derivatives-form');

    // Fill out the rest of the form and submit it.
    $edit = array();
    $edit['generate_missing'] = $force;
    foreach ((array) $dsids as $dsid) {
      $edit["dsids[$dsid]"] = TRUE;
    }
    if ($pids === TRUE) {
      $this->drupalPost($path, $edit, t('Regenerate selected derivatives for all collection objects'), array(), array(), 'islandora-batch-derivative-trigger-regenerate-derivatives-form');
    }
    else {
      $edit['children[]'] = $pids;
      $this->drupalPost($path, $edit, t('Regenerate selected derivatives for selected objects'), array(), array(), 'islandora-batch-derivative-trigger-regenerate-derivatives-form');
    }
  }

  /**
   * Fills out the batch metadata regeneration form and submits it.
   */
  protected function triggerBatchMetadata($collection_pid, $content_model, $mapping, $pids = TRUE) {
    // Regenerate the page based on content model.
    $path = "islandora/object/$collection_pid/manage/collection";
    $edit = array(
      'content_model' => $content_model,
    );
    $this->drupalPostAJAX($path, $edit, 'content_model', NULL, array(), array(), 'islandora-batch-derivative-trigger-regenerate-metadata-form');

    // Fill out the rest of the form and submit it.
    $edit = array();
    $edit['mapping'] = $mapping;
    if ($pids === TRUE) {
      $this->drupalPost($path, $edit, t('Regenerate DC metadata for all collection objects'), array(), array(), 'islandora-batch-derivative-trigger-regenerate-metadata-form');
    }
    else {
      $edit['children[]'] = $pids;
      $this->drupalPost($path, $edit, t('Regenerate DC metadata for selected objects'), array(), array(), 'islandora-batch-derivative-trigger-regenerate-metadata-form');
    }
  }
}
