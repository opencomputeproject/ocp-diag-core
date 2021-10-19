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

"""Unit tests for meltan.lib.testhelper.teststep."""

import unittest
from meltan.lib.testhelper import teststep


class DummyTest(teststep.TestStep):
  """Dummy TestStep class."""


class TestStepTest(unittest.TestCase):

  def testDefaultStepName(self):
    dummy_test_step = DummyTest()
    self.assertEqual(dummy_test_step.GetStepName(), 'DummyTest')

if __name__ == '__main__':
  unittest.main()
