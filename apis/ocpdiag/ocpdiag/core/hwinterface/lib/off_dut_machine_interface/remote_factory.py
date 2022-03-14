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

"""Remotefactory package creates a remote interface based on the flag input."""

from distutils import spawn

from absl import flags

from ocpdiag.core.hwinterface.lib.off_dut_machine_interface import remote
from ocpdiag.core.hwinterface.lib.off_dut_machine_interface.ssh import remote as ssh_remote

#
_MI_SERVICE_ADDR = flags.DEFINE_string(
    'mi_service_addr',
    '',
    'machine interface service address to talk to, if it is empty, the lib uses ssh lib by default.',
    allow_override_cpp=True)

_REMOTE_SSH_KEY_PATH = flags.DEFINE_string(
    'remote_ssh_key_path',
    '',
    'optional: needed if the ssh connection needs a private key',
    allow_override_cpp=True)

_REMOTE_SSH_TUNNEL_FILE_PATH = flags.DEFINE_string(
    'remote_ssh_tunnel_file_path',
    '',
    'optional: ssh control socket file to share the connection and the remote_ssh_tunnel_file_path is empty. Check SSH Multiplexing for more details.',
    allow_override_cpp=True)


def NewConn(node: remote.NodeSpec) -> remote.Conn:
  """NewConn creates a new machine node connection.

  Creates a new machine node connection to read/write remote files, or run
  remote commands on the specified machine node.

  Args:
    node: Contains the info of node to connect

  Returns:
    The created Conn

  Raises:
    ValueError: node.address is empty.
    NotImplementedError: Using machine interface which is not yet implemented.
  """

  if _MI_SERVICE_ADDR.value:
    raise NotImplementedError(
        'Machine interface is requested but not yet implemented.')
  if not node.address:
    raise ValueError('Machine node address must not be empty.')

  ssh_path = spawn.find_executable('ssh')
  if ssh_path is None:
    raise ValueError('Unable to find ssh executable')
  if ssh_path not in {'/bin/ssh', '/usr/bin/ssh'}:
    raise ValueError(f'SSH exec path ({ssh_path}) not among expected values.')

  return ssh_remote.SSHConn(
      node,
      ssh_key_path=_REMOTE_SSH_KEY_PATH.value,
      ssh_tunnel_file_path=_REMOTE_SSH_TUNNEL_FILE_PATH.value,
      ssh_bin_path=ssh_path)
