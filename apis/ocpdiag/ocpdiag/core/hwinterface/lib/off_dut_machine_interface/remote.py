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

"""Remote module defines the interface for a remote node connection.

The connection provides functions to read/write remote files, or run remote
commands on the specified machine node.
"""
import abc
import datetime
from typing import List
import six


class CommandResult(object):
  """CommandResult conatins the command result."""

  def __init__(self, exit_code: int, stdout: bytes, stderr: bytes):
    self.exit_code = exit_code
    self.stdout = stdout
    self.stderr = stderr


class CommandOption(object):
  """CommandOption redirects the command stdout and stderr.

  When stdout/stderr is set to a specified file, then stdout/stderr will be
  redirected to that file, and stdout/stderr in CommandResult will be empty.
  TODO(b/186697069): This feature is not yet supported.
  """

  def __init__(self, stdout_file: str, stderr_file: str):
    self.stdout_file = stdout_file
    self.stderr_file = stderr_file


class NodeSpec(object):
  """NodeSpec contains the machine node configuration."""

  def __init__(self, address: str):
    self.address = address


@six.add_metaclass(abc.ABCMeta)
class Conn:
  """Provides interface to read/write and run command on remote machine node."""

  def __init__(self, node: NodeSpec):
    self.node = node
    pass

  @abc.abstractmethod
  def ReadFile(self, path: str) -> bytes:
    """Reads the given path, raise an exception when read fails."""

  @abc.abstractmethod
  def WriteFile(self, path: str, data: bytes):
    """Writes data to given path, raise an exception when write fails."""

  @abc.abstractmethod
  def RunCommand(self, stdin: bytes, duration: datetime.timedelta,
                 args: List[str], opts: CommandOption) -> CommandResult:
    """RunCommand runs the specified command on the given machine node.

    RunCommand runs the specified command on the given machine node. And return
    the command result. If the command fails to finish within duration, it
    raise an exception.

    Args:
      stdin: Data to be sent to command through stdin.
      duration: Allowed duration to run the command.
      args: The command and its arguments to execute.
      opts: To control the stdout/stderr redirect.

    Returns:
      CommandResult contains the result of command execution.

    Raises:
      TimeoutError when the command failed to finish within duration.
    """
