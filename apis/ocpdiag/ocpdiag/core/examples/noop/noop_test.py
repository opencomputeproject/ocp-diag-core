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
"""Tests for noop."""
import os
import subprocess

import unittest
from ocpdiag.core.testing.file_utils import data_file_path_prefix


class NoopTest(unittest.TestCase):

  def _execute_and_check(self, binary_name):
    exec_path = f'ocpdiag/core/examples/noop/{binary_name}'
    tmpdir = os.getenv('TEST_TMPDIR')
    args = [os.path.join(data_file_path_prefix(), exec_path)]
    # If rc != 0, raises an exception.
    result = subprocess.run(
        args,
        timeout=30,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        env={'TMPDIR': tmpdir},
        check=True)
    self.assertEqual(len(result.stdout), 0)  # pylint: disable=g-generic-assert

  def test_noop(self):
    self._execute_and_check('noop')

  def test_noop_defaults(self):
    self._execute_and_check('noop_defaults')


if __name__ == '__main__':
  unittest.main()
