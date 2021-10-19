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

"""Tests for meltan.lib.params.utils."""

import io
import sys

from meltan.lib.params import utils
from meltan.lib.params.testdata import test_params_pb2
import unittest


class UtilsTest(unittest.TestCase):

  def test_json_parses_correctly(self):
    sys.stdin = io.StringIO("""{
      "foo" : "foovalue",
      "bar" : 42,
      "msg" : {
        "sub" : "submessage",
        "other" : [1, 2, 3]
      }
    }""")
    msg = utils.GetParams(test_params_pb2.Params())
    self.assertEqual(msg.foo, "foovalue")
    self.assertEqual(msg.bar, 42)
    self.assertEqual(msg.msg.sub, "submessage")
    self.assertEqual(msg.msg.other, [1, 2, 3])


if __name__ == "__main__":
  unittest.main()
