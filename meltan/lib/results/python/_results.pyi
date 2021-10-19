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

from typing import Text, List, Optional

from google.protobuf import struct_pb2
from google.protobuf import message
from meltan.lib.results import results_pb2


def SetResultsLibFlags(meltan_copy_results_to_stdout: bool,
                       meltan_results_filepath: Text, machine_under_test: Text,
                       meltan_strict_reporting: bool) -> None:
  ...


def InitTestRun(name: Text) -> TestRun:
  ...


class TestRun:

  def StartAndRegisterInfos(self,
                            dutinfos: List[DutInfo],
                            params: Optional[message.Message] = ...) -> None:
    ...

  def End(self) -> results_pb2.TestResult:
    ...

  def Skip(self) -> results_pb2.TestResult:
    ...

  def AddError(self, symptom: Text, message: Text) -> None:
    ...

  def AddTag(self, tag: Text) -> None:
    ...

  def Status(self) -> results_pb2.TestStatus:
    ...

  def Result(self) -> results_pb2.TestStatus:
    ...

  def Started(self) -> bool:
    ...

  def Ended(self) -> bool:
    ...

  def LogDebug(self, msg: Text) -> None:
    ...

  def LogInfo(self, msg: Text) -> None:
    ...

  def LogWarn(self, msg: Text) -> None:
    ...

  def LogError(self, msg: Text) -> None:
    ...

  def LogFatal(self, msg: Text) -> None:
    ...


def BeginTestStep(parent: TestRun, name: Text) -> TestStep:
  ...


class TestStep:

  def AddDiagnosis(self,
                   type: results_pb2.Diagnosis.Type,
                   symptom: Text,
                   message: Text,
                   records: List[HwRecord] = ...) -> None:
    ...

  def AddError(self,
               symptom: Text,
               message: Text,
               records: List[SwRecord] = ...) -> None:
    ...

  def AddMeasurement(self,
                     info: results_pb2.MeasurementInfo,
                     elem: results_pb2.MeasurementElement,
                     hwrec: Optional[HwRecord] = ...) -> None:
    ...

  def AddFile(self, file: results_pb2.File) -> None:
    ...

  def AddArtifactExtension(self, name: Text,
                           extension: message.Message) -> None:
    ...

  def LogDebug(self, msg: Text) -> None:
    ...

  def LogInfo(self, msg: Text) -> None:
    ...

  def LogWarn(self, msg: Text) -> None:
    ...

  def LogError(self, msg: Text) -> None:
    ...

  def LogFatal(self, msg: Text) -> None:
    ...

  def End(self) -> None:
    ...

  def Skip(self) -> None:
    ...

  def Ended(self) -> bool:
    ...

  def Status(self) -> int:
    ...

  def Id(self) -> Text:
    ...


class DutInfo:

  def __init__(self, name: Text):
    ...

  def AddHardware(self, info: results_pb2.HardwareInfo) -> HwRecord:
    ...

  def AddSoftware(self, info: results_pb2.SoftwareInfo) -> SwRecord:
    ...

  def AddPlatformInfo(self, info: Text) -> None:
    ...

  def Registered(self) -> bool:
    ...

  def ToProto(self) -> results_pb2.DutInfo:
    ...


class HwRecord:

  def Data() -> results_pb2.HardwareInfo:
    ...


class SwRecord:

  def Data() -> results_pb2.SoftwareInfo:
    ...


def BeginMeasurementSeries(
    parent: TestStep, hw: HwRecord,
    info: results_pb2.MeasurementInfo) -> MeasurementSeries:
  ...


class MeasurementSeries:

  def AddElement(self, value: struct_pb2.Value) -> None:
    ...

  def AddElementWithRange(self, val: struct_pb2.Value,
                          range: results_pb2.MeasurementElement.Range) -> None:
    ...

  def AddElementWithValues(self, val: struct_pb2.Value,
                           valid_values: List[struct_pb2.Value]) -> None:
    ...

  def Id(self) -> Text:
    ...

  def Ended(self) -> bool:
    ...

  def End(self) -> None:
    ...
