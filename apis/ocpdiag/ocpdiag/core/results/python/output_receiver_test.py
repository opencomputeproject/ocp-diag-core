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
    self.assertIsNotNone(output.test_run.start)
    self.assertIsNotNone(output.test_run.end)

    # Iterate through the output.
    cnt = 0
    for artifact in output.raw:
      self.assertTrue(artifact.HasField('test_run_artifact'))
      cnt += 1
    self.assertEqual(cnt, 2)  # 1 start + 1 end

    # Make sure the output filepath is populated.
    self.assertTrue(output.raw.file_path)


if __name__ == '__main__':
  unittest.main()
