// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <memory>

#include "absl/flags/parse.h"
#include "google/protobuf/struct.pb.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/types/span.h"
#include "ocpdiag/core/examples/simple/params.pb.h"
#include "ocpdiag/core/examples/simple/simple_lib.h"
#include "ocpdiag/core/params/utils.h"
#include "ocpdiag/core/results/results.h"
#include "ocpdiag/core/results/results.pb.h"

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

constexpr char kProceduralErrorSymptom[] =
    "my_test-procedural-error";
constexpr char kGoodHardware[] = "my hardware is good!";
constexpr char kFirstStep[] = "my_first_step";
constexpr char kSecondStep[] = "my_second_step";

void FillHwInfo(HardwareInfo* hw_info, const bool has_component) {
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

  // Parse the input parameters.
  ocpdiag::simple::Params params;
  auto status = ocpdiag::params::GetParams(&params);
  if (!status.ok()) {
    test_run->AddError(
        kProceduralErrorSymptom,
        absl::StrCat("Failed to parse parameters:", status.ToString()));
    return EXIT_FAILURE;
  }

  // Make a valid DutInfo and register it with the TestRun.
  DutInfo dut_info("TestHost");
  // Build the hw info.

  HardwareInfo hw_info_with_fru;

  FillHwInfo(&hw_info_with_fru, /* has_component = */false);
  HwRecord hw_record_with_fru = dut_info.AddHardware(hw_info_with_fru);

  HardwareInfo hw_info_with_component;
  FillHwInfo(&hw_info_with_component, /* has_component = */true);
  HwRecord hw_record_with_component =
      dut_info.AddHardware(hw_info_with_component);

  // Build the sw info.
  SoftwareInfo sw_info;
  sw_info.set_name("my_test");
  sw_info.set_arena("myArena");
  sw_info.set_version("myVersion");
  SwRecord sw_record = dut_info.AddSoftware(sw_info);

  // Add the platform info.
  dut_info.AddPlatformInfo("myInfo1");
  dut_info.AddPlatformInfo("myInfo2");

  std::vector<DutInfo> dutinfos;
  dutinfos.emplace_back(std::move(dut_info));

  // Now the TestRun officially "starts" and emits the TestRunStart artifact.
  test_run->StartAndRegisterInfos(dutinfos, params);
  // It's now possible to construct TestSteps using this fully initialized and
  // started TestRun.

  // Create our first TestStep.
  absl::StatusOr<std::unique_ptr<TestStep>> step1_or =
      api.BeginTestStep(test_run.get(), kFirstStep);
  if (!step1_or.ok()) {
    test_run->AddError(kProceduralErrorSymptom,
                       step1_or.status().ToString());
    return EXIT_FAILURE;
  }
  std::unique_ptr<TestStep> step1 = std::move(*step1_or);

  // Add all measurement types to step 1.
  ocpdiag::example::AddAllMeasurementTypes(step1.get(),
                                          &hw_record_with_component);

  // Create our second TestStep.
  absl::StatusOr<std::unique_ptr<TestStep>> step2_or =
      api.BeginTestStep(test_run.get(), kSecondStep);
  if (!step2_or.ok()) {
    test_run->AddError(kProceduralErrorSymptom,
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

  // Add example artifact extensions.
  step2->AddArtifactExtension("example-artifact-extension1",
                              hw_info_with_component);

  // Demonstrate passing diagnoses. Here we add one associated hw record
  // per diagnosis, but multiple records can be associated.
  step2->AddDiagnosis(
      ocpdiag::results_pb::Diagnosis_Type::Diagnosis_Type_PASS,
      "my_test-good-myHardwareWithFruLocation", kGoodHardware,
      {hw_record_with_fru});

  step2->AddDiagnosis(
      ocpdiag::results_pb::Diagnosis_Type::Diagnosis_Type_PASS,
      "my_test-good-myHardwareWithComponentLocation", kGoodHardware,
      {hw_record_with_component});

  // Demonstrate MeasurementSeries
  MeasurementInfo meas_info;
  meas_info.set_name("my_series");
  meas_info.set_unit("awesomeness 1-10");
  absl::StatusOr<std::unique_ptr<MeasurementSeries>> series_or =
      api.BeginMeasurementSeries(step2.get(), hw_record_with_fru, meas_info);
  if (!series_or.ok()) {
    step2->AddError(kProceduralErrorSymptom, series_or.status().ToString(),
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
