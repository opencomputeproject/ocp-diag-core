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

"""Base test class definition for testhelper."""

import platform
import typing

from google.protobuf import message
from meltan.lib.results import results_pb2
from meltan.lib.results.python import results
from meltan.lib.testhelper import teststep


class FailedError(Exception):
  """Exceptions to catch test error."""


class SkipError(Exception):
  """Exceptions to catch test error."""


class Test():
  """Base class for Meltan test."""

  def __init__(self):
    self._hwinfo_map = dict()
    self._swinfo_map = dict()
    self._steps_iter = iter([])
    self._runner = None
    self._params = None
    return

  def Setup(self, runner: results.TestRun, params: message.Message):
    self._runner = runner
    self._params = params
    return

  def NextStep(self) -> teststep.TestStep:
    return next(self._steps_iter)

  def GetHwInfoMap(self) -> typing.Dict[str, results_pb2.HardwareInfo]:
    return self._hwinfo_map

  def GetSwInfoMap(self) -> typing.Dict[str, results_pb2.SoftwareInfo]:
    return self._swinfo_map

  def GetDutName(self) -> str:
    return platform.node()
