# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

"""a python version of ocpdiag/core/examples/simple/simple.cc."""

import sys



from absl import app

from google.protobuf import struct_pb2
from google.protobuf import json_format
from google.protobuf import text_format
from ocpdiag.core.examples.simple import params_pb2
from ocpdiag.core.params import utils
from ocpdiag.core.results import results_pb2
from ocpdiag.core.results.python import results
from third_party.pybind11_abseil import status as error


def main(argv) -> None:  # pylint: disable=unused-argument
  try:
    runner = results.InitTestRun("myTest")
  except error.StatusNotOk as e:
    print("Init Test Run Failed %s " % e, file=sys.stderr)
    return

  runner.LogInfo("Initialized the test!")

  # Parse the input parameters.
  try:
    params = utils.GetParams(params_pb2.Params())  # pylint: disable=unused-variable
  except json_format.ParseError as e:
    runner.AddError("my_test-procedural-error",
                    "Failed to parse parameters:{0}".format(e))
    return

  dut_info = results.DutInfo("TestHost")
  hw_info = results_pb2.HardwareInfo(
      arena="myArena",
      name="myName",
      manufacturer="myManufacturer",
      mfg_part_number="myMfgPartNum",
      part_type="myPartType")

  hw_record = dut_info.AddHardware(hw_info)
  runner.StartAndRegisterInfos([dut_info], params)

  # Make a DutInfo with HW, but forget to register it
  unused_dut_info = results.DutInfo("UnregisteredHost")
  bad_hw_info = results_pb2.HardwareInfo(
      arena="badArena",
      name="badName",
      manufacturer="badManufacturer",
      mfg_part_number="badMfgPartNum",
      part_type="badPartType")
  unregistered_record = unused_dut_info.AddHardware(bad_hw_info)

  try:
    step = results.BeginTestStep(runner, "MyStep")
  except error.StatusNotOk as e:
    runner.AddError("my_test-procedural-error", e)
    return

  # Demonstrate Diagnosis with good/bad HwRecord
  #
  # Adding one registered HwRecord, and one unregistered.
  # This will illustrate that using an unregistered HwRecord
  # emits an Error artifact and the HwRecord will not be
  # referenced in the Diagnosis result
  step.AddDiagnosis(results_pb2.Diagnosis.PASS, "my_test-good-myHardware",
                    "my hardware is good!", [hw_record, unregistered_record])

  # Demonstrate MeasurementSeries
  meas_info = results_pb2.MeasurementInfo(
      name="MySeries", unit="awesomeness 1-10")
  try:
    series = results.BeginMeasurementSeries(step, hw_record, meas_info)
  except error.StatusNotOk as e:
    print(e, file=sys.stderr)
    return

  val_max = struct_pb2.Value(number_value=10)
  val_min = struct_pb2.Value(number_value=0)
  new_range = results_pb2.MeasurementElement.Range(
      maximum=val_max, minimum=val_min)
  series.AddElementWithRange(val_max, new_range)
  series.End()

  meas_info.name = "another series"
  try:
    series = results.BeginMeasurementSeries(step, hw_record, meas_info)
  except error.StatusNotOk as e:
    print(e, file=sys.stderr)
    return

if __name__ == "__main__":
  app.run(main)
