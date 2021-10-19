# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Tests for simple examples."""
import os
import subprocess

from google.protobuf import json_format
import unittest
from absl.testing import parameterized
from meltan.lib.results import results_pb2
from meltan.lib.testing.file_utils import data_file_path_prefix


class ExamplePyTest(parameterized.TestCase):

  def _execute_and_collect_output(self, binary_name):
    exec_path = 'meltan/core/examples/simple/{:s}'.format(
        binary_name)
    args = [os.path.join(data_file_path_prefix(), exec_path)]
    # If rc != 0, raises an exception.
    result = subprocess.run(
        args,
        timeout=30,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
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
