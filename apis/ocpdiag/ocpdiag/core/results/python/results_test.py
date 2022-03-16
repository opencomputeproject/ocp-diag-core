# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

"""Tests for ocpdiag.core.results.python.results."""

import faulthandler
import io
import unittest

from google.protobuf import empty_pb2
from google.protobuf import struct_pb2
from google.protobuf import timestamp_pb2
from google3.net.proto2.contrib.pyutil import compare
from google.protobuf import json_format
from google.protobuf import text_format
from ocpdiag.core.results import results_pb2
from ocpdiag.core.results.python import results
from ocpdiag.core.results.python import resultstestpy

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
    want = text_format.Parse(
        """hostname: "sitd"
           hardware_components {
             hardware_info_id: "0"
             name: "hardware_part"
           }""", results_pb2.DutInfo())
    compare.assertProto2Equal(self, want, out)

  def testDutInfoSwInfo(self):
    dutinfo = results.DutInfo("sitd")
    swinfo = results_pb2.SoftwareInfo()
    swinfo.name = "linux"
    dutinfo.AddSoftware(swinfo)
    out = dutinfo.ToProto()
    want = text_format.Parse(
        """hostname: "sitd"
           software_infos {
             software_info_id: "0"
             name: "linux"
           }""", results_pb2.DutInfo())
    compare.assertProto2Equal(self, want, out)

  def testDutInfoPlatform(self):
    dutinfo = results.DutInfo("sitd")
    dutinfo.AddPlatformInfo("something")
    out = dutinfo.ToProto()
    want = text_format.Parse(
        """hostname: "sitd"
           platform_info {
             info: "something"
           }""", results_pb2.DutInfo())
    compare.assertProto2Equal(self, want, out)


class TestRunTest(unittest.TestCase):

  def setUp(self):
    super().setUp()
    self.results_test = resultstestpy.ResultsTest()
    self.runner = self.results_test.InitializeTestRun("TestRunTest")

  def tearDown(self):
    super().tearDown()
    _ = self.runner.End()

  def testTestRunEnd(self):
    self.assertFalse(self.runner.Ended())
    self.runner.StartAndRegisterInfos([])
    result = self.runner.End()
    self.assertTrue(self.runner.Ended())
    self.assertEqual(result, results_pb2.TestResult.PASS)
    self.assertEqual(self.runner.Status(), results_pb2.TestStatus.COMPLETE)

  def testTestRunSkip(self):
    self.assertEqual(self.runner.Status(), results_pb2.TestStatus.UNKNOWN)
    result = self.runner.Skip()
    self.assertEqual(self.runner.Status(), results_pb2.TestStatus.SKIPPED)
    self.assertEqual(result, results_pb2.TestResult.NOT_APPLICABLE)

  def testTestRunAddError(self):
    self.runner.AddError("symptom", "msg")
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    compare.assertProto2Contains(
        self,
        """test_run_artifact { error { symptom: "symptom" msg: "msg"} }""", got)

  def testTestRunAddTag(self):
    tag = "Guten Tag"
    self.runner.AddTag(tag)
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    compare.assertProto2Contains(
        self, """test_run_artifact { tag { tag: "Guten Tag" } }""", got)

  def testTestRunDebug(self):
    msg = "my house has termites, please debug it"
    self.runner.LogDebug(msg)
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    compare.assertProto2Contains(
        self, """test_run_artifact {
      log { severity: DEBUG text: "my house has termites, please debug it" }
    }""", got)

  def testTestRunInfo(self):
    msg = "my house has termites, please debug it"
    self.runner.LogInfo(msg)
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    compare.assertProto2Contains(
        self, """test_run_artifact {
      log { severity: INFO text: "my house has termites, please debug it" }
    }""", got)

  def testTestRunWraning(self):
    msg = "my house has termites, please debug it"
    self.runner.LogWarn(msg)
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    compare.assertProto2Contains(
        self, """test_run_artifact {
      log { severity: WARNING text: "my house has termites, please debug it" }
    }""", got)

  def testTestRunFatal(self):
    msg = "my house has termites, please debug it"
    self.runner.LogFatal(msg)
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    compare.assertProto2Contains(
        self, """test_run_artifact {
      log { severity: FATAL text: "my house has termites, please debug it" }
    }""", got)

  def testTestRunStartAndRegisterInfos(self):
    self.assertFalse(self.runner.Started())
    d0 = results.DutInfo("host0")
    hw = d0.AddHardware(
        text_format.Parse(HWREGISTERED, results_pb2.HardwareInfo()))
    sw = d0.AddSoftware(
        text_format.Parse(SWREGISTERED, results_pb2.SoftwareInfo()))
    d0.AddPlatformInfo("plugin:something")
    # pay attention d0, d1 not valid after this call, this may not good
    self.runner.StartAndRegisterInfos([d0])
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    hwinfo = HWREGISTERED % hw.Data().hardware_info_id
    swinfo = SWREGISTERED % sw.Data().software_info_id
    want = """test_run_artifact {
          test_run_start {
            name: "TestRunTest"
            parameters {
              type_url: "type.googleapis.com/google.protobuf.Empty"
            }
            dut_info {
              hostname: "host0"
              hardware_components { %s }
              software_infos { %s }
              platform_info { info: "plugin:something" }
            }
          }
        }
        sequence_number: 0"""
    compare.assertProto2Contains(self, want % (hwinfo, swinfo), got)


class TestRunStep(unittest.TestCase):

  def setUp(self):
    super().setUp()
    self.results_test = resultstestpy.ResultsTest()
    self.runner = self.results_test.InitializeTestRun("TestRunTest")
    self.step = self.results_test.CreateTestStep(self.runner, "TestStepTest",
                                                 "0")

  def tearDown(self):
    super().tearDown()
    self.step.End()
    _ = self.runner.End()

  def testTestStepBegin(self):
    self.runner.StartAndRegisterInfos([])
    results.BeginTestStep(self.runner, "BeginTest")
    buf = io.StringIO(self.results_test.GetJsonReadableOutput())
    # skip first line
    buf.readline()
    line1 = buf.readline()
    got = json_format.Parse(line1, results_pb2.OutputArtifact())
    _ = self.runner.End()
    compare.assertProto2Contains(
        self, """test_step_artifact {
      test_step_start { name: "BeginTest" }
      test_step_id: "0"
    }""", got)

  def testTestStepAddDiagnosis(self):
    dutinfo = results.DutInfo("hostname")
    hw = dutinfo.AddHardware(
        text_format.Parse(HWREGISTERED, results_pb2.HardwareInfo()))
    self.results_test.RegisterHwId(hw.Data().hardware_info_id)
    self.step.AddDiagnosis(results_pb2.Diagnosis.PASS, "symptom",
                           "add diag success", [hw])
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    want = """test_step_artifact {
              diagnosis {
                symptom: "symptom"
                type: PASS
                msg: "add diag success"
                hardware_info_id: "%s"
              }
           }
           sequence_number: 0""" % (
               hw.Data().hardware_info_id)
    compare.assertProto2Contains(
        self, want, got, ignored_fields=["test_step_artifact.test_step_id"])

  def testTestStepAddDiagnosisWithoutHwRecord(self):
    self.step.AddDiagnosis(results_pb2.Diagnosis.PASS, "symptom",
                           "add diag success")
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    want = """test_step_artifact {
              diagnosis {
                symptom: "symptom"
                type: PASS
                msg: "add diag success"
              }
           }
           sequence_number: 0"""
    compare.assertProto2Contains(
        self, want, got, ignored_fields=["test_step_artifact.test_step_id"])

  def testTestStepAddError(self):
    dutinfo = results.DutInfo("hostname")
    sw = dutinfo.AddSoftware(
        text_format.Parse(SWREGISTERED, results_pb2.SoftwareInfo()))
    self.results_test.RegisterSwId(sw.Data().software_info_id)
    self.step.AddError("symptom", "add error success", [sw])
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    want = """test_step_artifact {
              error {
                symptom: "symptom"
                msg: "add error success"
                software_info_id: "%s"
              }
            }
            sequence_number: 0""" % (
                sw.Data().software_info_id)
    compare.assertProto2Contains(
        self, want, got, ignored_fields=["test_step_artifact.test_step_id"])

  def testTestStepAddErrorWithoutSwRecord(self):
    self.step.AddError("symptom", "add error success")
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    want = """test_step_artifact {
              error {
                symptom: "symptom"
                msg: "add error success"
              }
            }
            sequence_number: 0"""
    compare.assertProto2Contains(
        self, want, got, ignored_fields=["test_step_artifact.test_step_id"])

  def testTestStepAddMeasurement(self):
    dutinfo = results.DutInfo("hostname")
    val = struct_pb2.Value(number_value=3.14)
    now = timestamp_pb2.Timestamp()
    elem = results_pb2.MeasurementElement(value=val, dut_timestamp=now)
    elem.valid_values.values.append(val)
    hw = dutinfo.AddHardware(
        text_format.Parse(HWREGISTERED, results_pb2.HardwareInfo()))
    self.results_test.RegisterHwId(hw.Data().hardware_info_id)
    info = results_pb2.MeasurementInfo(name="measurement info", unit="unit")
    self.step.AddMeasurement(info, elem, hw)
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    want = """test_step_artifact {
          measurement {
            info {
              name: "measurement info"
              unit: "unit"
              hardware_info_id: "%s"
            }
            element {
              measurement_series_id: ""
              value { number_value: 3.14 }
              valid_values { values { number_value: 3.14 } }
            }
          }
        }""" % (hw.Data().hardware_info_id)
    compare.assertProto2Contains(
        self,
        want,
        got,
        ignored_fields=[
            "test_step_artifact.test_step_id",
            "test_step_artifact.measurement.element.dut_timestamp"
        ])

  def testTestStepAddMeasurementWithNullptr(self):
    val = struct_pb2.Value(number_value=3.14)
    now = timestamp_pb2.Timestamp()
    elem = results_pb2.MeasurementElement(value=val, dut_timestamp=now)
    elem.valid_values.values.append(val)
    info = results_pb2.MeasurementInfo(name="measurement info", unit="unit")
    self.step.AddMeasurement(info, elem, None)
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    want = """test_step_artifact {
          measurement {
            info {
              name: "measurement info"
              unit: "unit"
            }
            element {
              measurement_series_id: ""
              value { number_value: 3.14 }
              valid_values { values { number_value: 3.14 } }
            }
          }
        }"""
    compare.assertProto2Contains(
        self,
        want,
        got,
        ignored_fields=[
            "test_step_artifact.test_step_id",
            "test_step_artifact.measurement.element.dut_timestamp"
        ])

  def testTestStepAddMeasurementWithDefualt(self):
    val = struct_pb2.Value(number_value=3.14)
    now = timestamp_pb2.Timestamp()
    elem = results_pb2.MeasurementElement(value=val, dut_timestamp=now)
    elem.valid_values.values.append(val)
    info = results_pb2.MeasurementInfo(name="measurement info", unit="unit")
    self.step.AddMeasurement(info, elem)
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    want = """test_step_artifact {
          measurement {
            info {
              name: "measurement info"
              unit: "unit"
            }
            element {
              measurement_series_id: ""
              value { number_value: 3.14 }
              valid_values { values { number_value: 3.14 } }
            }
          }
        }"""
    compare.assertProto2Contains(
        self,
        want,
        got,
        ignored_fields=[
            "test_step_artifact.test_step_id",
            "test_step_artifact.measurement.element.dut_timestamp"
        ])

  def testTestStepAddFile(self):
    file = results_pb2.File(
        upload_as_name="upload name",
        output_path="out path",
        description="description",
        content_type="content type")
    self.step.AddFile(file)
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    want = """test_step_artifact {
          file {
            upload_as_name: "upload name"
            output_path: "out path"
            description: "description"
            content_type: "content type"
          }
        }"""
    compare.assertProto2Contains(
        self, want, got, ignored_fields=[
            "test_step_artifact.test_step_id",
        ])

  def testTestStepAddArtifactExtension(self):
    extension = empty_pb2.Empty()
    self.step.AddArtifactExtension("test extension", extension)
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    want = """test_step_artifact {
          extension {
            name: "test extension"
            extension {
              type_url: "type.googleapis.com/google.protobuf.Empty"
              value: "%s"
            }
          }
        }
        sequence_number: 0""" % (
            extension.SerializeToString())
    compare.assertProto2Contains(
        self,
        want,
        got,
        ignored_fields=[
            "test_step_artifact.test_step_id",
            "test_step_artifact.extension.extension"
        ])

  def testTestStepEnd(self):
    self.assertFalse(self.step.Ended())
    self.assertEqual(self.step.Status(), results_pb2.TestStatus.UNKNOWN)
    self.step.End()
    self.assertTrue(self.step.Ended())
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    want = """test_step_artifact {
          test_step_end { name: "TestStepTest" status: COMPLETE }
        }
        sequence_number: 0"""
    compare.assertProto2Contains(
        self, want, got, ignored_fields=[
            "test_step_artifact.test_step_id",
        ])

  def testTestStepSkip(self):
    self.assertEqual(self.step.Status(), results_pb2.TestStatus.UNKNOWN)
    self.step.Skip()
    self.assertEqual(self.step.Status(), results_pb2.TestStatus.SKIPPED)

  def testTestStepDebug(self):
    msg = "my house has termites, please debug it"
    self.step.LogDebug(msg)
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    want = """test_step_artifact {
          log { severity: DEBUG text: "my house has termites, please debug it" }
        }
        sequence_number: 0"""
    compare.assertProto2Contains(
        self, want, got, ignored_fields=[
            "test_step_artifact.test_step_id",
        ])

  def testTestStepInfo(self):
    msg = "my house has termites, please debug it"
    self.step.LogInfo(msg)
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    want = """test_step_artifact {
          log { severity: INFO text: "my house has termites, please debug it" }
        }
        sequence_number: 0"""
    compare.assertProto2Contains(
        self, want, got, ignored_fields=[
            "test_step_artifact.test_step_id",
        ])

  def testTestStepWraning(self):
    msg = "my house has termites, please debug it"
    self.step.LogWarn(msg)
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    want = """test_step_artifact {
          log { severity: WARNING text: "my house has termites, please debug it" }
        }
        sequence_number: 0"""
    compare.assertProto2Contains(
        self, want, got, ignored_fields=[
            "test_step_artifact.test_step_id",
        ])

  def testTestStepFatal(self):
    msg = "my house has termites, please debug it"
    self.step.LogFatal(msg)
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    want = """test_step_artifact {
          log { severity: FATAL text: "my house has termites, please debug it" }
        }
        sequence_number: 0"""
    compare.assertProto2Contains(
        self, want, got, ignored_fields=[
            "test_step_artifact.test_step_id",
        ])


class TestMeasurementSeries(unittest.TestCase):

  def setUp(self):
    super().setUp()
    self.results_test = resultstestpy.ResultsTest()
    info = text_format.Parse(MEASUREMENTINFO, results_pb2.MeasurementInfo())
    self.series = self.results_test.CreateMeasurementSeries(info)

  def tearDown(self):
    super().tearDown()
    self.series.End()

  def testMeasurementSeriesBegin(self):
    step = self.results_test.CreateTestStep(None, "TestMeasurementSeries", "0")
    d0 = results.DutInfo("host")
    hw = d0.AddHardware(
        text_format.Parse(HWREGISTERED, results_pb2.HardwareInfo()))
    hw_id = hw.Data().hardware_info_id
    self.results_test.RegisterHwId(hw_id)
    infostr = MEASUREMENTINFO % (hw_id)
    info = text_format.Parse(infostr, results_pb2.MeasurementInfo())
    series = results.BeginMeasurementSeries(step, hw, info)
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    step.End()
    series.End()
    want = """test_step_artifact {
           measurement_series_start { measurement_series_id: "%s" info { %s } }
           test_step_id: "%s"
         }
        sequence_number: 0""" % (series.Id(), infostr, step.Id())
    compare.assertProto2Contains(self, want, got)

  def testMeasurementSeriesAddElement(self):
    val = struct_pb2.Value(number_value=3.14)
    self.series.AddElement(val)
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    want = """test_step_artifact {
          measurement_element {
            measurement_series_id: "%s"
            value { number_value: 3.14 }
          }
        }""" % (
            self.series.Id())
    compare.assertProto2Contains(
        self, want, got, ignored_fields=["test_step_artifact.test_step_id"])

  def testMeasurementSeriesAddElementWithRange(self):
    val_max = struct_pb2.Value(number_value=3.15)
    val_min = struct_pb2.Value(number_value=3.13)
    val = struct_pb2.Value(number_value=3.14)
    nrange = results_pb2.MeasurementElement.Range(
        maximum=val_max, minimum=val_min)
    self.series.AddElementWithRange(val, nrange)
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    want = """test_step_artifact {
          measurement_element {
            measurement_series_id: "%s"
            value { number_value: 3.14 }
            range {
              maximum { number_value: 3.15 }
              minimum { number_value: 3.13 }
            }
          }
        }""" % (
            self.series.Id())
    compare.assertProto2Contains(
        self, want, got, ignored_fields=["test_step_artifact.test_step_id"])

  def testMeasurementSeriesAddElementWithValues(self):
    val = struct_pb2.Value(number_value=3.14)
    self.series.AddElementWithValues(val, [val])
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    want = """test_step_artifact {
          measurement_element {
            measurement_series_id: "%s"
            value { number_value: 3.14 }
            valid_values { values { number_value: 3.14 } }
          }
        }""" % (
            self.series.Id())
    compare.assertProto2Contains(
        self, want, got, ignored_fields=["test_step_artifact.test_step_id"])

  def testMeasurementSeriesEnd(self):
    self.assertFalse(self.series.Ended())
    self.series.End()
    self.assertTrue(self.series.Ended())
    got = json_format.Parse(self.results_test.GetJsonReadableOutput(),
                            results_pb2.OutputArtifact())
    want = """
    test_step_artifact {
      measurement_series_end {
        measurement_series_id: "%s"
        total_measurement_count: 0
        }
      }""" % (
          self.series.Id())
    compare.assertProto2Contains(
        self, want, got, ignored_fields=["test_step_artifact.test_step_id"])


if __name__ == "__main__":
  faulthandler.enable()
  unittest.main()
