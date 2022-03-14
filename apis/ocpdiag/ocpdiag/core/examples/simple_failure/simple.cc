// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <memory>

#include "absl/flags/parse.h"
#include "google/protobuf/struct.pb.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/types/span.h"
#include "ocpdiag/core/examples/simple_failure/params.pb.h"
#include "ocpdiag/core/examples/simple_failure/simple_lib.h"
#include "ocpdiag/core/params/utils.h"
#include "ocpdiag/core/results/results.h"
#include "ocpdiag/core/results/results.pb.h"

//

using ::google::protobuf::Value;
using ::ocpdiag::results::DutInfo;
using ::ocpdiag::results::HwRecord;
using ::ocpdiag::results::MeasurementSeries;
using ::ocpdiag::results::SwRecord;
using ::ocpdiag::results::TestRun;
using ::ocpdiag::results::TestStep;
using ::ocpdiag::results_pb::ComponentLocation;
using ::ocpdiag::results_pb::File;
using ::ocpdiag::results_pb::HardwareInfo;
using ::ocpdiag::results_pb::MeasurementElement;
using ::ocpdiag::results_pb::MeasurementInfo;
using ::ocpdiag::results_pb::SoftwareInfo;

void FillHwInfo(HardwareInfo* hw_info, bool has_component) {
  hw_info->set_arena("myArena");
  hw_info->set_name("myName");
  hw_info->set_manufacturer("myManufacturer");
  hw_info->set_mfg_part_number("myMfgPartNum");
  hw_info->set_part_type("myPartType");

  ComponentLocation* fru_location = hw_info->mutable_fru_location();
  fru_location->set_serial_number("myFruSerial");
  fru_location->set_blockpath("MyFruBlockpath");
  fru_location->set_odata_id("MyFruOdataId");
  fru_location->set_devpath("MyFruDevpath");

  if (has_component) {
    ComponentLocation* component_location =
        hw_info->mutable_component_location();
    component_location->set_serial_number("myComponentSerial");
    component_location->set_blockpath("MyComponentBlockpath");
    component_location->set_odata_id("MyComponentOdataId");
    component_location->set_devpath("MyComponentDevpath");
  }
}

std::string CreateTextFile() {
  const char* path = "simple_ocpdiag_test_file.txt";
  std::ofstream file(path);
  file << "Simple OCPDiag test file content.\n";
  file.close();
  return path;
}

int main(int argc, char* argv[]) {
  // Initialize the Google context.
  absl::ParseCommandLine(argc, argv);

  ocpdiag::results::ResultApi api;

  // Initialize the TestRun object.
  absl::StatusOr<std::unique_ptr<TestRun>> test_run_or =
      api.InitializeTestRun("myTest");
  if (!test_run_or.ok()) {
    // This is the only place in the code where we need to write directly to
    // STDERR (because the TestRun object was not initialized).
    // Once we have an initialized TestRun, it is best practice to use
    // `TestRun::Error()` or `TestStep::Error()` instead.
    std::cerr << test_run_or.status().ToString() << std::endl;
    return EXIT_FAILURE;
  }
  std::unique_ptr<TestRun> test_run = std::move(*test_run_or);
  test_run->LogInfo("Initialized the test!");
  test_run->LogError("Example error log.");
  test_run->LogWarn("Example warning log.");
  test_run->AddTag("test-run-start tag");
  test_run->AddError("test-run-error", "Example test run error");

  // Parse the input parameters.
  ocpdiag::simple_failure::Params params;
  auto status = ocpdiag::params::GetParams(&params);
  if (!status.ok()) {
    test_run->AddError(
        "my_test-procedural-error",
        absl::StrCat("Failed to parse parameters:", status.ToString()));
    return EXIT_FAILURE;
  }

  // Make a valid DutInfo and register it with the TestRun.
  DutInfo dut_info("TestHost");
  // Build the hw info.

  HardwareInfo hw_info_with_fru;
  FillHwInfo(&hw_info_with_fru, false);
  HwRecord hw_record_with_fru = dut_info.AddHardware(hw_info_with_fru);

  HardwareInfo hw_info_with_component;
  FillHwInfo(&hw_info_with_component, true);
  HwRecord hw_record_with_component =
      dut_info.AddHardware(hw_info_with_component);

  // Build the sw info.
  SoftwareInfo sw_info;
  sw_info.set_name("my_test");
  sw_info.set_arena("myArena");
  sw_info.set_version("myVersion");
  SwRecord sw_record = dut_info.AddSoftware(sw_info);
  std::vector<DutInfo> dutinfos;
  dutinfos.emplace_back(std::move(dut_info));

  // Make a DutInfo with HW, but forget to register it. Note we forget to add
  // this to the `dutinfos` vector, so it never gets registered with the
  // TestRun and any HwRecords associated with it will not be usable for
  // diagnostics.
  DutInfo unused_dut_info("UnregisteredHost");
  HardwareInfo bad_hw_info;
  bad_hw_info.set_arena("badArena");
  bad_hw_info.set_name("badName");
  bad_hw_info.set_manufacturer("badManufacturer");
  bad_hw_info.set_mfg_part_number("badMfgPartNum");
  bad_hw_info.set_part_type("badPartType");
  HwRecord unregistered_record = unused_dut_info.AddHardware(bad_hw_info);

  // Now the TestRun officially "starts" and emits the TestRunStart artifact.
  test_run->StartAndRegisterInfos(dutinfos, params);
  // It's now possible to construct TestSteps using this fully initialized and
  // started TestRun.

  // Create our first TestStep.
  absl::StatusOr<std::unique_ptr<TestStep>> step1_or =
      api.BeginTestStep(test_run.get(), "my_first_step");
  if (!step1_or.ok()) {
    test_run->AddError("my_test-procedural-error",
                       step1_or.status().ToString());
    return EXIT_FAILURE;
  }
  std::unique_ptr<TestStep> step1 = std::move(*step1_or);

  // Add all measurement types to step 1.
  ocpdiag::example::AddAllMeasurementTypes(step1.get(),
                                          &hw_record_with_component);

  // Create our second TestStep.
  absl::StatusOr<std::unique_ptr<TestStep>> step2_or =
      api.BeginTestStep(test_run.get(), "my_second_step");
  if (!step2_or.ok()) {
    test_run->AddError("my_test-procedural-error",
                       step2_or.status().ToString());
    return EXIT_FAILURE;
  }
  std::unique_ptr<TestStep> step2 = std::move(*step2_or);

  // Add an example File.
  File file;
  file.set_upload_as_name("test_file");
  file.set_output_path(CreateTextFile());
  file.set_description("This is a test file :)");
  file.set_content_type("text/plain");
  file.add_tags()->set_tag("ocpdiag_example");
  step2->AddFile(std::move(file));

  // Add an example tag during step run.
  test_run->AddTag("during-test-step tag");

  // Add example artifact extentions.
  step2->AddArtifactExtension("example-artifact-extention1",
                              hw_info_with_component);
  step2->AddArtifactExtension("example-artifact-extention2", bad_hw_info);

  // Demonstrate a Diagnosis with good/bad HwRecord.
  //
  // Here we add one registered HwRecord, and one unregistered.
  // This illustrates that using an unregistered HwRecord
  // emits an Error artifact and the HwRecord will not be
  // referenced in the Diagnosis result.
  step2->AddDiagnosis(
      ocpdiag::results_pb::Diagnosis_Type::Diagnosis_Type_PASS,
      "my_test-good-myHardwareUnregistered", "my hardware is good!",
      {unregistered_record});

  step2->AddDiagnosis(
      ocpdiag::results_pb::Diagnosis_Type::Diagnosis_Type_PASS,
      "my_test-good-myHardwareWithFruLocation", "my hardware is good!",
      {hw_record_with_fru});

  step2->AddDiagnosis(
      ocpdiag::results_pb::Diagnosis_Type::Diagnosis_Type_PASS,
      "my_test-good-myHardwareWithComponentLocation", "my hardware is good!",
      {hw_record_with_component});

  // Demonstrate MeasurementSeries
  MeasurementInfo meas_info;
  meas_info.set_name("my_series");
  meas_info.set_unit("awesomeness 1-10");
  absl::StatusOr<std::unique_ptr<MeasurementSeries>> series_or =
      api.BeginMeasurementSeries(step2.get(), hw_record_with_fru, meas_info);
  if (!series_or.ok()) {
    step2->AddError("my_test-procedural-error", series_or.status().ToString(),
                    {sw_record});
    return EXIT_FAILURE;
  }
  std::unique_ptr<MeasurementSeries> series = std::move(*series_or);
  Value val_max, val_min, val;
  val_max.set_number_value(10);
  val_min.set_number_value(1);
  val.set_number_value(10);
  MeasurementElement::Range range;
  *range.mutable_maximum() = val_max;
  *range.mutable_minimum() = val_min;
  series->AddElementWithRange(val_max, range);
  series->End();  // user may explicitly End a Series, Step, or TestRun.

  meas_info.set_name("another_series");
  absl::StatusOr<std::unique_ptr<MeasurementSeries>> series2 =
      api.BeginMeasurementSeries(step2.get(), hw_record_with_component,
                                 meas_info);

  // See that series2, TestStep and TestRun emit End artifacts when they go out
  // of scope.
  return EXIT_SUCCESS;
}

//
// //depot/ocpdiag/g3doc/codelab.md)
