# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

"""Tests for ocpdiag.core.params.utils."""

import io
import sys

import unittest
from ocpdiag.core.params import utils
from ocpdiag.core.params.testdata import test_params_pb2


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
