# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

"""Tests for simple examples."""
import os
import subprocess

from google.protobuf import json_format
import unittest
from absl.testing import parameterized
from ocpdiag.core.results import results_pb2
from ocpdiag.core.testing.file_utils import data_file_path_prefix


class ExamplePyTest(parameterized.TestCase):

  def _execute_and_collect_output(self, binary_name):
    exec_path = 'ocpdiag/core/examples/simple/{:s}'.format(
        binary_name)
    args = [os.path.join(data_file_path_prefix(), exec_path)]
    # If rc != 0, raises an exception.
    result = subprocess.run(
        args,
        timeout=30,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=True)
    self._lines = result.stdout.decode('utf-8').split('\n')

  def _check_artifact_validity(self):
    for line in self._lines:
      #
      if line.strip() and 'testRunStart' not in line:
        json_format.Parse(line, results_pb2.OutputArtifact())

  @parameterized.named_parameters(('c++', 'simple_bin'),
                                  ('python', 'python/simple'))
  def test_executables(self, exe):
    self._execute_and_collect_output(exe)
    self._check_artifact_validity()


if __name__ == '__main__':
  unittest.main()
