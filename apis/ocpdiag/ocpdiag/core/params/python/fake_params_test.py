# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

"""Tests for fake_params."""

from google3.net.proto2.contrib.pyutil import compare
from ocpdiag.core.params import utils
from ocpdiag.core.params.python import fake_params
import unittest
from ocpdiag.core.params.testdata import test_params_pb2


class FakeParamsTest(unittest.TestCase):

  def test_fake_params(self):
    params = test_params_pb2.Params()
    params.foo = 'foo'
    with fake_params.FakeParams(params):
      got = utils.GetParams(test_params_pb2.Params())

    compare.assertProto2Equal(self, got, params)


if __name__ == '__main__':
  unittest.main()
