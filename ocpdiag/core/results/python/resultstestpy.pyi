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

from typing import Text, Optional

from ocpdiag.lib.results.python import results
from ocpdiag.lib.results import results_pb2


class ResultsTest:

  def InitializeTestRun(self, name: Text) -> results.TestRun:
    ...

  def CreateTestStep(self, runner: Optional[results.TestRun], name: Text,
                     customId: Text) -> results.TestStep:
    ...

  def CreateMeasurementSeries(
      self, info: results_pb2.MeasurementInfo) -> results.MeasurementSeries:
    ...

  def RegisterHwId(self, i: Text) -> None:
    ...

  def RegisterSwId(self, i: Text) -> None:
    ...

  def GetJsonReadableOutput() -> Text:
    ...
