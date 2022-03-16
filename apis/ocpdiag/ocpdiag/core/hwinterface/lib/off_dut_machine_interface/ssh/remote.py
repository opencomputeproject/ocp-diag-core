# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

"""Remote module implements the Conn interface using ssh.

This module implements the Conn interface defined in
//third_party/ocpdiag/core/hwinterface/lib/off_dut_machine_interface/remote.py
using ssh.
"""

import datetime
import subprocess
import sys
from typing import List

from absl import logging

from ocpdiag.core.hwinterface.lib.off_dut_machine_interface import remote


class SSHConn(remote.Conn):
  """SSHConn implements remote.Conn interface using ssh.

  This class implements the Conn interface defined in
  //third_party/ocpdiag/core/hwinterface/lib/off_dut_machine_interface/remote.py
  using ssh.
  """

  SSH_DEFAULT_USER = 'root'
  DEFAULT_IO_TIMEOUT = datetime.timedelta(minutes=15)

  def __init__(self, node: remote.NodeSpec, ssh_key_path: str,
               ssh_tunnel_file_path: str, ssh_bin_path: str):
    super().__init__(node)
    self.ssh_key_path = ssh_key_path
    self.ssh_tunnel_file_path = ssh_tunnel_file_path
    self.ssh_bin_path = ssh_bin_path

  def ReadFile(self, path: str) -> bytes:
    """ReadFile reads the content of the file specified by path."""

    result = self.RunCommand(self.DEFAULT_IO_TIMEOUT, bytes(), ['cat', path])
    if result.exit_code == 0:
      return result.stdout
    raise IOError(result.exit_code)

  def WriteFile(self, path: str, data: bytes):
    """WriteFile writes the data to the file specified by path."""

    result = self.RunCommand(self.DEFAULT_IO_TIMEOUT, data, ['cat', '>', path])
    if result.exit_code != 0:
      raise IOError(result.exit_code)

  def RunCommand(
      self,
      duration: datetime.timedelta,
      stdin: bytes,
      args: List[str],
      opts: remote.CommandOption = remote.CommandOption(
          stdout_file='', stderr_file='')
  ) -> remote.CommandResult:
    """Run the command by ssh to the remote server."""
    if opts.stdout_file or opts.stderr_file:
      raise NotImplementedError(
          'Stdout/stderr redirection is requested but not yet implemented.')

    ssh_args = self.GenerateSshArgs(args)

    try:
      completed_process = subprocess.run(
          args=ssh_args,
          input=stdin,
          stdout=subprocess.PIPE,
          stderr=subprocess.PIPE,
          timeout=duration.total_seconds(),
          check=False)
      return remote.CommandResult(
          exit_code=completed_process.returncode,
          stdout=completed_process.stdout,
          stderr=completed_process.stderr)
    except subprocess.TimeoutExpired:
      logging.warn('Command failed to finish within %f seconds.',
                   duration.total_seconds())
      raise TimeoutError
    except:
      logging.warn('Command failed to run: %s.', sys.exc_info()[0])
      raise RuntimeError(sys.exc_info()[0])

  def GenerateSshArgs(self, command: List[str]) -> List[str]:
    """Helper function to generate the args list with ssh details."""

    ssh_args = command
    ssh_args.insert(0, self.ssh_bin_path)
    ssh_args.insert(1, self.SSH_DEFAULT_USER + '@' + self.node.address)
    if self.ssh_key_path:
      ssh_args.insert(2, '-i')
      ssh_args.insert(3, self.ssh_key_path)
    if self.ssh_tunnel_file_path:
      ssh_args.insert(2, '-S')
      ssh_args.insert(3, self.ssh_tunnel_file_path)
    return ssh_args
