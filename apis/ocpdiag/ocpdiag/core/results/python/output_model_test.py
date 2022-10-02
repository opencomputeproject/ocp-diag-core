# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

"""Tests for output_model."""

import unittest
from absl import flags

from ocpdiag.core.results import results_pb2
from ocpdiag.core.results.python import output_model

FLAGS = flags.FLAGS


class OutputModelTest(unittest.TestCase):

  def test_model(self):
    model = output_model.TestRunOutput()

    artifact = results_pb2.OutputArtifact()
    artifact.test_run_artifact.test_run_start.name = 'TestRun'
    model.AddOutputArtifact(artifact)

    self.assertIsNotNone(model.start)

  # This test touches all data members to ensure they are wrapped.
  def test_all_members(self):
    model = output_model.TestRunOutput()

    # Add a test step so we can test that object too.
    artifact = results_pb2.OutputArtifact()
    artifact.test_step_artifact.test_step_start.name = 'TestStep'
    model.AddOutputArtifact(artifact)

    _ = model.start
    _ = model.end
    _ = model.logs
    _ = model.errors
    _ = model.tags
    self.assertEqual(len(model.steps.keys()), 1)
    step = list(model.steps.values())[0]
    _ = step.start
    _ = step.end
    _ = step.errors
    _ = step.files
    _ = step.artifact_extensions
    _ = step.measurements
    _ = step.measurement_series
    _ = step.diagnoses


if __name__ == '__main__':
  unittest.main()
