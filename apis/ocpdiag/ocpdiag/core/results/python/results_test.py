# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

"""Tests for ocpdiag.core.results.python.results."""

import faulthandler
import unittest

from absl.testing import absltest
from google.protobuf import empty_pb2
from google.protobuf import struct_pb2
from google.protobuf import timestamp_pb2
from google.protobuf import text_format
from ocpdiag.core.results import results_pb2
from ocpdiag.core.results.python import output_receiver
from ocpdiag.core.results.python import results

HWREGISTERED = """
  hardware_info_id: "%s"
  arena: "arena"
  name: "name"
  fru_location {
    devpath: "fru devpath"
    odata_id: "fru odata_id"
    blockpath: "fru blockpath"
    serial_number: "fru serial_number"
  }
  part_number: "part_number"
  manufacturer: "manufacturer"
  mfg_part_number: "mfg_part_number"
  part_type: "part_type"
  component_location {
    devpath: "component devpath"
    odata_id: "component odata_id"
    blockpath: "component blockpath"
  }"""

SWREGISTERED = """
  software_info_id: "%s"
  arena: "arena"
  name: "name"
  version: "sw version"
  """

MEASUREMENTINFO = """
  name: "measurement info" unit: "unit" hardware_info_id: "%s"
  """


class DutInfoTest(unittest.TestCase):

  def testDutInfoHwInfo(self):
    dutinfo = results.DutInfo("sitd")
    hwinfo = results_pb2.HardwareInfo()
    hwinfo.name = "hardware_part"
    dutinfo.AddHardware(hwinfo)
    out = dutinfo.ToProto()
    self.assertEqual(out.hardware_components[0].name, "hardware_part")

  def testDutInfoSwInfo(self):
    dutinfo = results.DutInfo("sitd")
    swinfo = results_pb2.SoftwareInfo()
    swinfo.name = "linux"
    dutinfo.AddSoftware(swinfo)
    out = dutinfo.ToProto()
    self.assertEqual(out.software_infos[0].name, "linux")

  def testDutInfoPlatform(self):
    dutinfo = results.DutInfo("sitd")
    dutinfo.AddPlatformInfo("something")
    out = dutinfo.ToProto()
    self.assertEqual(out.platform_info.info, ["something"])


# A base class that collects the test result output.
class ResultsTestBase(unittest.TestCase):

  def setUp(self):
    super().setUp()
    self.output = output_receiver.OutputReceiver()
    self.test_run = results.TestRun("ResultsTest", self.output.artifact_writer)


class TestRunTest(ResultsTestBase):

  def testTestRunEnd(self):
    with self.test_run as test_run:
      self.assertFalse(test_run.Ended())
      test_run.StartAndRegisterInfos([])
      result = test_run.End()
      self.assertTrue(test_run.Ended())
      self.assertEqual(result, results_pb2.TestResult.PASS)
      self.assertEqual(test_run.Status(), results_pb2.TestStatus.COMPLETE)

  def testTestRunSkip(self):
    with self.test_run as test_run:
      self.assertEqual(test_run.Status(), results_pb2.TestStatus.UNKNOWN)
      result = test_run.Skip()
      self.assertEqual(test_run.Status(), results_pb2.TestStatus.SKIPPED)
      self.assertEqual(result, results_pb2.TestResult.NOT_APPLICABLE)

  def testTestRunAddError(self):
    with self.test_run as test_run:
      test_run.AddError("symptom", "msg")

    self.assertEqual(len(self.output.model.errors), 1)
    self.assertEqual(self.output.model.errors[0].symptom, "symptom")

  def testTestRunAddTag(self):
    with self.test_run as test_run:
      test_run.AddTag("T")

    self.assertEqual(len(self.output.model.tags), 1)
    self.assertEqual(self.output.model.tags[0].tag, "T")

  def testTestRunLogs(self):
    with self.test_run as test_run:
      test_run.LogDebug("A")
      test_run.LogInfo("B")
      test_run.LogWarn("C")
      test_run.LogError("D")
      test_run.LogFatal("E")

    self.assertEqual(len(self.output.model.logs), 5)
    self.assertEqual(self.output.model.logs[results_pb2.Log.DEBUG][0].text, "A")
    self.assertEqual(self.output.model.logs[results_pb2.Log.INFO][0].text, "B")
    self.assertEqual(self.output.model.logs[results_pb2.Log.WARNING][0].text,
                     "C")
    self.assertEqual(self.output.model.logs[results_pb2.Log.ERROR][0].text, "D")
    self.assertEqual(self.output.model.logs[results_pb2.Log.FATAL][0].text, "E")

  def testTestRunStartAndRegisterInfos(self):
    with self.test_run as test_run:
      self.assertFalse(test_run.Started())
      d0 = results.DutInfo("host0")
      d0.AddHardware(
          text_format.Parse(HWREGISTERED, results_pb2.HardwareInfo()))
      d0.AddSoftware(
          text_format.Parse(SWREGISTERED, results_pb2.SoftwareInfo()))
      d0.AddPlatformInfo("plugin:something")
      # pay attention d0, d1 not valid after this call, this may not good
      test_run.StartAndRegisterInfos([d0])

    self.assertEqual(
        len(self.output.model.start.dut_info[0].hardware_components), 1)
    self.assertEqual(len(self.output.model.start.dut_info[0].software_infos), 1)


class TestRunStep(ResultsTestBase):

  def testTestStepBegin(self):
    with self.test_run as test_run:
      test_run.StartAndRegisterInfos([])
      test_run.BeginTestStep("TestStepTest")

    self.assertIn("0", self.output.model.steps.keys())
    self.assertEqual(self.output.model.steps["0"].start.name, "TestStepTest")

  def testTestStepAddDiagnosis(self):
    with self.test_run as test_run:
      dutinfo = results.DutInfo("hostname")
      hw = dutinfo.AddHardware(
          text_format.Parse(HWREGISTERED, results_pb2.HardwareInfo()))
      test_run.StartAndRegisterInfos([dutinfo])

      with test_run.BeginTestStep("TestStepTest") as step:
        step.AddDiagnosis(results_pb2.Diagnosis.PASS, "symptom",
                          "add diag success", [hw])

    self.assertEqual(self.output.model.steps["0"].diagnoses[0].type,
                     results_pb2.Diagnosis.PASS)

  def testTestStepAddError(self):
    with self.test_run as test_run:
      dutinfo = results.DutInfo("hostname")
      sw = dutinfo.AddSoftware(
          text_format.Parse(SWREGISTERED, results_pb2.SoftwareInfo()))
      test_run.StartAndRegisterInfos([dutinfo])

      with test_run.BeginTestStep("TestStepTest") as step:
        step.AddError("symptom", "add error success", [sw])
    errors = self.output.model.steps["0"].errors
    self.assertEqual(len(errors), 1)
    self.assertEqual(errors[0].msg, "add error success")

  def testTestStepAddMeasurement(self):
    with self.test_run as test_run:
      dutinfo = results.DutInfo("hostname")
      hw = dutinfo.AddHardware(
          text_format.Parse(HWREGISTERED, results_pb2.HardwareInfo()))
      test_run.StartAndRegisterInfos([dutinfo])

      val = struct_pb2.Value(number_value=3.14)
      now = timestamp_pb2.Timestamp()
      elem = results_pb2.MeasurementElement(value=val, dut_timestamp=now)
      elem.valid_values.values.append(val)

      info = results_pb2.MeasurementInfo(name="measurement info", unit="unit")
      with test_run.BeginTestStep("TestStepTest") as step:
        step.AddMeasurement(info, elem, hw)

    measurements = self.output.model.steps["0"].measurements
    self.assertEqual(len(measurements), 1)
    self.assertEqual(measurements[0].info.name, "measurement info")

  def testTestStepAddFile(self):
    with self.test_run as test_run:
      test_run.StartAndRegisterInfos([])
      with test_run.BeginTestStep("TestStepTest") as step:
        step.AddFile(
            results_pb2.File(
                upload_as_name="upload name",
                output_path="out path",
                description="description",
                content_type="content type"))

    files = self.output.model.steps["0"].files
    self.assertEqual(len(files), 1)
    self.assertEqual(files[0].upload_as_name, "upload name")

  def testTestStepAddArtifactExtension(self):
    extension = empty_pb2.Empty()
    with self.test_run as test_run:
      test_run.StartAndRegisterInfos([])
      with test_run.BeginTestStep("TestStepTest") as step:
        step.AddArtifactExtension("test extension", extension)

    extensions = self.output.model.steps["0"].artifact_extensions
    self.assertEqual(len(extensions), 1)
    self.assertEqual(extensions[0].name, "test extension")

  def testTestStepSkip(self):
    with self.test_run as test_run:
      test_run.StartAndRegisterInfos([])
      with test_run.BeginTestStep("TestStepTest") as step:
        self.assertEqual(step.Status(), results_pb2.TestStatus.UNKNOWN)
        step.Skip()
        self.assertEqual(step.Status(), results_pb2.TestStatus.SKIPPED)

  def testTestStepDebug(self):
    with self.test_run as test_run:
      test_run.StartAndRegisterInfos([])
      with test_run.BeginTestStep("TestStepTest") as step:
        step.LogDebug("A")
        step.LogInfo("B")
        step.LogWarn("C")
        step.LogError("D")
        step.LogFatal("E")

    logs = self.output.model.steps["0"].logs
    self.assertEqual(len(logs), 5)
    self.assertEqual(logs[results_pb2.Log.DEBUG][0].text, "A")
    self.assertEqual(logs[results_pb2.Log.INFO][0].text, "B")
    self.assertEqual(logs[results_pb2.Log.WARNING][0].text, "C")
    self.assertEqual(logs[results_pb2.Log.ERROR][0].text, "D")
    self.assertEqual(logs[results_pb2.Log.FATAL][0].text, "E")


class TestMeasurementSeries(ResultsTestBase):

  def testMeasurementSeriesBegin(self):
    with self.test_run as test_run:
      dut_info = results.DutInfo("host")
      hw = dut_info.AddHardware(
          text_format.Parse(HWREGISTERED, results_pb2.HardwareInfo()))
      hw_id = hw.Data().hardware_info_id
      test_run.StartAndRegisterInfos([dut_info])
      with test_run.BeginTestStep("TestMeasurementSeries") as step:
        infostr = MEASUREMENTINFO % (hw_id)
        info = text_format.Parse(infostr, results_pb2.MeasurementInfo())
        with step.BeginMeasurementSeries(hw, info) as measurement_series:
          measurement_series.AddElement(struct_pb2.Value(number_value=3.14))

    measurement_series = self.output.model.steps["0"].measurement_series
    self.assertEqual(len(measurement_series), 1)
    series_output = list(measurement_series.values())[0]
    self.assertEqual(series_output.start.info.name, "measurement info")
    self.assertEqual(len(series_output.measurement_elements), 1)
    self.assertEqual(series_output.measurement_elements[0].value.number_value,
                     3.14)


if __name__ == "__main__":
  faulthandler.enable()
  absltest.main()
