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

"""Tests for ocpdiag.core.hwinterface.lib.off_dut_machine_interface.remote_factory."""

from distutils import spawn
import sys
import unittest
from unittest import mock

from absl import flags
from absl.testing import flagsaver

from ocpdiag.core.hwinterface.lib.off_dut_machine_interface import remote
from ocpdiag.core.hwinterface.lib.off_dut_machine_interface import remote_factory


class RemoteFactoryTest(unittest.TestCase):

  def setUp(self):
    super().setUp()
    self._saved_flags = flagsaver.save_flag_values()

  def tearDown(self):
    flagsaver.restore_flag_values(self._saved_flags)
    super().tearDown()

  def test_newconn_throws_when_given_machine_interface_address(self):
    flags.FLAGS.mi_service_addr = 'machine_interface_address'
    node_spec = remote.NodeSpec(address='dut')
    with self.assertRaises(NotImplementedError):
      remote_factory.NewConn(node_spec)

  def test_newconn_throws_when_nodespac_address_empty(self):
    node_spec = remote.NodeSpec(address='')
    with self.assertRaises(ValueError):
      remote_factory.NewConn(node_spec)

  def test_newconn_create_conn(self):
    node_spec = remote.NodeSpec(address='dut')
    conn = remote_factory.NewConn(node_spec)
    self.assertIsNotNone(conn)

  @mock.patch.object(spawn, 'find_executable', autospec=True)
  def test_newconn_path_none(self, mock_find):
    node_spec = remote.NodeSpec(address='dut')
    mock_find.return_value = None
    with self.assertRaises(ValueError):
      remote_factory.NewConn(node_spec)

  @mock.patch.object(spawn, 'find_executable', autospec=True)
  def test_newconn_path_invalid(self, mock_find):
    node_spec = remote.NodeSpec(address='dut')
    mock_find.return_value = '/invalid/path/ssh'
    with self.assertRaises(ValueError):
      remote_factory.NewConn(node_spec)


if __name__ == '__main__':
  flags.FLAGS(sys.argv)
  unittest.main()
