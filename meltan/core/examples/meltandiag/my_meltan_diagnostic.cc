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

#include "meltan/core/examples/meltandiag/my_meltan_diagnostic.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "google/protobuf/struct.pb.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "meltan/lib/results/results.h"
#include "meltan/lib/results/results.pb.h"

using ::meltan::results::DutInfo;
using ::meltan::results::HwRecord;
using ::meltan::results::MeasurementSeries;
using ::meltan::results::TestRun;
using ::meltan::results::TestStep;

namespace rpb = ::meltan::results_pb;

rpb::TestResult MyMeltanDiagnostic::TakesTestRun(TestRun* runner) {
  runner->LogInfo("log info");
  return runner->End();
}

void MyMeltanDiagnostic::TakesTestStep(TestStep* step,
                                       absl::Span<HwRecord> records) {
  for (const HwRecord& rec : records) {
    step->AddDiagnosis(rpb::Diagnosis::PASS, "pass diag", "msg", {rec});
  }
}

void MyMeltanDiagnostic::TakesMeasurementSeries(MeasurementSeries* ms) {
  ms->AddElementWithRange(google::protobuf::Value(),
                          rpb::MeasurementElement::Range());
}

int MyMeltanDiagnostic::ExecuteTest() {
  absl::StatusOr<std::unique_ptr<TestRun>> runner_or =
      api()->InitializeTestRun("myTest");
  if (!runner_or.ok()) {
    std::cerr << "TestRun init failed: " << runner_or.status().ToString()
              << std::endl;
    return EXIT_FAILURE;
  }
  std::unique_ptr<TestRun> runner = std::move(runner_or.value());

  DutInfo dutInfo("MyHost");
  HwRecord rec = dutInfo.AddHardware(rpb::HardwareInfo());
  runner->StartAndRegisterInfos({std::move(dutInfo)});
  absl::StatusOr<std::unique_ptr<TestStep>> step_or =
      api()->BeginTestStep(&(*runner), "myStep");
  if (!step_or.ok()) {
    runner->LogError(absl::StrFormat("TestStep creation failed: %s",
                                     step_or.status().ToString()));
    return EXIT_FAILURE;
  }
  std::unique_ptr<TestStep> step = std::move(step_or.value());
  step->LogInfo("Step began");
  absl::StatusOr<std::unique_ptr<MeasurementSeries>> series_or =
      api()->BeginMeasurementSeries(&(*step), rec, rpb::MeasurementInfo());
  if (!series_or.ok()) {
    runner->LogError(absl::StrFormat("MeasurementSeries creation failed: %s",
                                     series_or.status().ToString()));
    return EXIT_FAILURE;
  }
  std::unique_ptr<MeasurementSeries> series = std::move(series_or.value());
  series->AddElementWithRange(google::protobuf::Value(),
                              rpb::MeasurementElement::Range());

  return EXIT_SUCCESS;
}
