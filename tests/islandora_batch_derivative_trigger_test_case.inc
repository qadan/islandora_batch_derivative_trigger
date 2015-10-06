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
    parent::setUp(array('islandora', 'batch_derivative_test'));
    $this->testCollection = 'test:' . $this->randomName();
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
   *   Whether or not to add a DERIVE datastream.
   */
  protected function ingestBatchDerivativeTestObject($mods, $obj, $derive) {
    // Base properties.
    $properties = array(
      'pid' => 'test',
      'label' => 'Test Object',
      'models' => array('islandora:batch_testCM'),
      'owner' => $this->loggedInUser,
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

    // Ingest, then derive DERIVE datastream if we were asked.
    $object = $this->ingestConstructedObject($properties, $datastreams);
    if ($derive) {
      batch_derivative_test_derive_derived_datastream($object, FALSE);
    }

    // Return the object.
    return $object;
  }
}
