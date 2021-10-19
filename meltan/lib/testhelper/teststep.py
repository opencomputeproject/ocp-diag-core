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

"""Base teststep class definition for testhelper."""

import typing

from meltan.lib.results import results_pb2
from meltan.lib.results.python import results


class FailedError(Exception):
  """Exceptions to catch test step error."""


class SkipError(Exception):
  """Exceptions to skip test step."""


class TestStep():
  """Base class for Meltan test setup."""

  def __init__(self):
    self._runner = None
    self._step = None
    self._hwrecord_map = None
    self._swrecord_map = None
    return

  def Setup(self, runner: results.TestRun, step: results.TestStep,
            hwrecord_map: typing.Dict[str, results.HwRecord],
            swrecord_map: typing.Dict[str, results.SwRecord]):
    self._runner = runner
    self._step = step
    self._hwrecord_map = hwrecord_map
    self._swrecord_map = swrecord_map
    return

  def AddMeasurement(self, key, value, unit):
    measurement_info = results_pb2.MeasurementInfo(name=key, unit=unit)
    measurement_element = results_pb2.MeasurementElement(value=value)
    self._step.AddMeasurement(measurement_info, measurement_element, None)

  def GetStepName(self):
    return type(self).__name__

  def Run(self):
    return
