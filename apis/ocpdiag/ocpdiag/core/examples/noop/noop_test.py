# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

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
