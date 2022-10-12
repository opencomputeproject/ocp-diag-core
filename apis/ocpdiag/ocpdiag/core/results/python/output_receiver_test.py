# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

"""Tests for output_receiver."""

import unittest

from ocpdiag.core.results.python import output_receiver
from ocpdiag.core.results.python import results


class OutputReceiverTest(unittest.TestCase):

  def test_model_inside_context(self):
    output = output_receiver.OutputReceiver()
    with results.TestRun('Test', output.artifact_writer):
      pass

    # Test that the model is populated.
    self.assertIsNotNone(output.model.start)
    self.assertIsNotNone(output.model.end)

    # Iterate through the output.
    cnt = 0
    for artifact in output:
      self.assertTrue(artifact.HasField('test_run_artifact'))
      cnt += 1
    self.assertEqual(cnt, 2)  # 1 start + 1 end


if __name__ == '__main__':
  unittest.main()
