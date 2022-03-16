# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

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
