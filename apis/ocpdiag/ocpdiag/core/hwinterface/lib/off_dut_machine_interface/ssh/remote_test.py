# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

"""Tests for ocpdiag.core.hwinterface.lib.off_dut_machine_interface.ssh.remote."""

import datetime
import signal
import subprocess
from unittest import mock

import unittest
from absl.testing import parameterized
from ocpdiag.core.hwinterface.lib.off_dut_machine_interface import remote
from ocpdiag.core.hwinterface.lib.off_dut_machine_interface.ssh import remote as ssh_remote


class RemoteTest(parameterized.TestCase):

  def setUp(self):
    super().setUp()
    self.addCleanup(mock.patch.stopall)
    self.run_fn = mock.patch.object(subprocess, 'run', autospec=True).start()
    self.run_fn.return_value = subprocess.CompletedProcess(
        args='',
        returncode=0,
        stdout=bytes([0x1, 0xaa, 0xff]),
        stderr='stderr'.encode())
    self.conn = ssh_remote.SSHConn(
        remote.NodeSpec(address='dut'),
        ssh_key_path='',
        ssh_tunnel_file_path='',
        ssh_bin_path='/bin/ssh')

  def test_runcommand_calls_subprocess_run(self):
    result = self.conn.RunCommand(
        duration=datetime.timedelta(minutes=15),
        stdin=bytes(),
        args=['ls', '-l'])

    # Verify the result.
    self.assertIsNotNone(result)
    self.assertEqual(0, result.exit_code)
    self.assertEqual(bytes([0x1, 0xaa, 0xff]), result.stdout)
    self.assertEqual('stderr'.encode(), result.stderr)

    # Verify correct argument to subprocess.run.
    self.run_fn.assert_called_once_with(
        args=['/bin/ssh', 'root@dut', 'ls', '-l'],
        input=bytes(),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=900,
        check=False)

  def test_readfile_calls_subprocess_run(self):
    result = self.conn.ReadFile('/to_read')

    # Verify the result.
    self.assertEqual(bytes([0x1, 0xaa, 0xff]), result)

    # Verify correct argument to subprocess.run.
    self.run_fn.assert_called_once_with(
        args=['/bin/ssh', 'root@dut', 'cat', '/to_read'],
        input=bytes(),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=900,
        check=False)

  def test_writefile_calls_subprocess_run(self):
    self.conn.WriteFile(path='/to_write', data=bytes([0x3, 0xff, 0x1d]))

    # Verify correct argument to subprocess.run.
    self.run_fn.assert_called_once_with(
        args=['/bin/ssh', 'root@dut', 'cat', '>', '/to_write'],
        input=bytes([0x3, 0xff, 0x1d]),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=900,
        check=False)

  def test_ssh_key_path_is_used(self):
    self.conn = ssh_remote.SSHConn(
        remote.NodeSpec(address='dut'),
        ssh_key_path='the_ssh_key_path',
        ssh_tunnel_file_path='',
        ssh_bin_path='/bin/ssh')
    self.conn.ReadFile(path='/to_read')

    # Verify correct argument to subprocess.run.
    self.run_fn.assert_called_once_with(
        args=[
            '/bin/ssh', 'root@dut', '-i', 'the_ssh_key_path', 'cat', '/to_read'
        ],
        input=bytes(),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=900,
        check=False)

  def test_ssh_tunnel_file_path_is_used(self):
    self.conn = ssh_remote.SSHConn(
        remote.NodeSpec(address='dut'),
        ssh_key_path='',
        ssh_tunnel_file_path='the_ssh_tunnel_file_path',
        ssh_bin_path='/bin/ssh')
    self.conn.ReadFile(path='/to_read')

    # Verify correct argument to subprocess.run.
    self.run_fn.assert_called_once_with(
        args=[
            '/bin/ssh', 'root@dut', '-S', 'the_ssh_tunnel_file_path', 'cat',
            '/to_read'
        ],
        input=bytes(),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=900,
        check=False)

  def test_readfile_raise_when_read_fail(self):
    self.run_fn.return_value = subprocess.CompletedProcess(
        args='', returncode=-signal.SIGKILL, stdout=bytes(), stderr=bytes())
    with self.assertRaises(IOError):
      self.conn.ReadFile(path='/to_read')

  def test_readfile_raise_when_write_fail(self):
    self.run_fn.return_value = subprocess.CompletedProcess(
        args='', returncode=-signal.SIGKILL, stdout=bytes(), stderr=bytes())
    with self.assertRaises(IOError):
      self.conn.WriteFile(path='/to_write', data=bytes([0x1]))

  @parameterized.named_parameters(('redirect_stdout', 'std_out', ''),
                                  ('redirect_stderr', '', 'std_err'))
  def test_runcommand_throws_for_redirect(self, stdout_file, stderr_file):
    with self.assertRaises(NotImplementedError):
      self.conn.RunCommand(
          duration=datetime.timedelta(minutes=15),
          stdin=bytes(),
          args=['ls', '-l'],
          opts=remote.CommandOption(
              stdout_file=stdout_file, stderr_file=stderr_file))

  def test_runcommand_throws_when_timeout(self):
    self.run_fn.side_effect = subprocess.TimeoutExpired(cmd='ls -l', timeout=1)
    with self.assertRaises(TimeoutError):
      self.conn.RunCommand(
          duration=datetime.timedelta(seconds=1),
          stdin=bytes(),
          args=['ls', '-l'])

  def test_runcommand_throws_when_fail(self):
    self.run_fn.side_effect = subprocess.SubprocessError()
    with self.assertRaises(RuntimeError):
      self.conn.RunCommand(
          duration=datetime.timedelta(seconds=1),
          stdin=bytes(),
          args=['ls', '-l'])


if __name__ == '__main__':
  unittest.main()
