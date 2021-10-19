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

#ifndef MELTAN_CORE_EXAMPLES_MELTANDIAG_MY_MELTAN_DIAGNOSTIC_H_
#define MELTAN_CORE_EXAMPLES_MELTANDIAG_MY_MELTAN_DIAGNOSTIC_H_

#include "absl/types/span.h"
#include "meltan/lib/meltan_diagnostic.h"
#include "meltan/lib/results/results.h"
#include "meltan/lib/results/results.pb.h"

// An example Diagnostic written using the Meltan APIs.
class MyMeltanDiagnostic : public meltan::MeltanDiagnostic {
 public:
  MyMeltanDiagnostic(meltan::results::ResultApi* api) : MeltanDiagnostic(api) {}

  int ExecuteTest() override;

  // Takes pointer to TestRun, allowing dependency injection in unit tests.
  meltan::results_pb::TestResult TakesTestRun(
      meltan::results::TestRun*);
  // Takes pointer to TestStep, allowing dependency injection in unit tests.
  void TakesTestStep(meltan::results::TestStep*,
                     absl::Span<meltan::results::HwRecord>);
  // Takes pointer to MeasurementSeries, allowing dependency injection in unit
  // tests.
  void TakesMeasurementSeries(meltan::results::MeasurementSeries*);
  // Method to demonstrate faked HwRecords for unit tests.
  void TakesRecords(absl::Span<meltan::results::HwRecord>);
};

#endif  // MELTAN_CORE_EXAMPLES_MELTANDIAG_MY_MELTAN_DIAGNOSTIC_H_
