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

#include <signal.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/parse.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "ocpdiag/core/examples/monitor/params.pb.h"
#include "ocpdiag/core/params/utils.h"
#include "ocpdiag/core/results/results.h"
#include "ocpdiag/core/results/results.pb.h"

using ::ocpdiag::core::examples::monitor::Params;
using ::ocpdiag::params::GetParams;
using ::ocpdiag::results::TestRun;
using ::ocpdiag::results::TestStep;

static volatile sig_atomic_t signal_received_ = false;
static void SignalHandler(int) { signal_received_ = true; }

// This will run the shim as a standalone commandline app.
int main(int argc, char* argv[]) {
  absl::ParseCommandLine(argc, argv);
  ocpdiag::results::ResultApi api;
  // Create Test Run Logger.
  absl::StatusOr<std::unique_ptr<TestRun>> test_run_or =
      api.InitializeTestRun("example_monitor");

  if (!test_run_or.ok()) {
    std::cerr << "[Test Internal Error]: Fail to creat TestRun object: "
              << test_run_or.status();
    return EXIT_FAILURE;
  }
  std::unique_ptr<TestRun> test_run = std::move(*test_run_or);
  test_run->LogInfo("Initiated TestRun");

  // Get parameters.
  Params params;
  if (const auto status = ocpdiag::params::GetParams(&params); !status.ok()) {
    test_run->AddError(
        "monitor-procedural-error",
        absl::StrFormat("Failed to parse parameters: %s", status.ToString()));
    return EXIT_FAILURE;
  }

  test_run->StartAndRegisterInfos(std::vector<ocpdiag::results::DutInfo>{},
                                     params);

  signal(SIGTERM, SignalHandler);
  signal(SIGINT, SignalHandler);

  int iter = 0;
  while (!signal_received_) {
    // Start one step.
    absl::StatusOr<std::unique_ptr<TestStep>> step_or =
        api.BeginTestStep(test_run.get(), absl::StrFormat("step-%d", iter));
    if (!step_or.ok()) {
      test_run->AddError("monitor-procedural-error",
                            absl::StrFormat("Failed to create step %d: %s",
                                            iter, step_or.status().ToString()));
      return EXIT_FAILURE;
    }
    std::unique_ptr<TestStep> step = std::move(*step_or);
    step->LogInfo(absl::StrFormat("Created step %d", iter));
    absl::SleepFor(absl::Seconds(1));
    step->End();
    iter++;
  }

  return EXIT_SUCCESS;
}
