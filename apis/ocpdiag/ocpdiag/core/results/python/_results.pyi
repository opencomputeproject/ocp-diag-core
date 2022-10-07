# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

from typing import Callable, Text, List, Optional

from google.protobuf import struct_pb2
from google.protobuf import message
from ocpdiag.core.results import results_pb2
from ocpdiag.core.results.python import output_model


def SetResultsLibFlags(
    ocpdiag_copy_results_to_stdout: bool,
    ocpdiag_results_filepath: Text,
    machine_under_test: Text,
    alsologtoocpdiagresults: bool,
    ocpdiag_strict_reporting: bool,
) -> None:
  ...


# DEPRECATED: Use TestRun().
def InitTestRun(name: Text) -> TestRun:
  ...


class MeasurementSeries:

  def __enter__(self) -> MeasurementSeries:
    ...

  def __exit__(self, *args):
    ...

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


class TestStep:

  def __enter__(self) -> TestStep:
    ...

  def __exit__(self, *args):
    ...

  def BeginMeasurementSeries(
      hwrec: HwRecord, info: results_pb2.MeasurementInfo) -> MeasurementSeries:
    ...

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


class TestRun:

  def __init__(self, name: str):
    ...

  def __enter__(self) -> TestRun:
    ...

  def __exit__(self, *args):
    ...

  def StartAndRegisterInfos(self,
                            dutinfos: List[DutInfo],
                            params: Optional[message.Message] = ...) -> None:
    ...

  def BeginTestStep(name: str) -> TestStep:
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


# DEPRECATED: Use TestRun.BeginTestStep().
def BeginTestStep(parent: TestRun, name: Text) -> TestStep:
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


# DEPRECATED: Use TestStep.BeginMeasurementSeries.
def BeginMeasurementSeries(
    parent: TestStep, hw: HwRecord,
    info: results_pb2.MeasurementInfo) -> MeasurementSeries:
  ...


class OutputReceiver:

  def __init__(self):
    ...

  def __enter__(self) -> OutputReceiver:
    ...

  def __exit__(self, *args):
    ...

  @property
  def model(self) -> output_model.TestRunOutput:
    ...

  def Iterate(callback: Callable[[results_pb2.OutputArtifact], bool]):
    ...
