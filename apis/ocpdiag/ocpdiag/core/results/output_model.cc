// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/output_model.h"

#include <fstream>
#include <tuple>

#include "absl/status/status.h"
#include "ocpdiag/core/compat/status_macros.h"
#include "ocpdiag/core/results/results.pb.h"

namespace ocpdiag::results {
namespace {

using ::ocpdiag::results_pb::OutputArtifact;
using ::ocpdiag::results_pb::TestRunArtifact;
using ::ocpdiag::results_pb::TestStepArtifact;

absl::Status HandleTestRunArtifact(TestRunOutput &data,
                                   OutputArtifact artifact) {
  TestRunArtifact &test_run = *artifact.mutable_test_run_artifact();
  if (test_run.has_test_run_start()) {
    if (data.start.has_value()) {
      return absl::UnknownError("Multiple TestRunStart artifacts");
    }
    data.start = test_run.test_run_start();
  } else if (test_run.has_test_run_end()) {
    if (data.end.has_value()) {
      return absl::UnknownError("Multiple TestRunEnd artifacts");
    }
    data.end = test_run.test_run_end();
  } else if (test_run.has_log()) {
    data.logs[test_run.log().severity()].emplace_back(test_run.log());
  } else if (test_run.has_tag()) {
    data.tags.push_back(test_run.tag());
  } else if (test_run.has_error()) {
    data.errors.push_back(test_run.error());
  }
  return absl::OkStatus();
}

absl::Status HandleTestStepArtifact(TestRunOutput &data,
                                    OutputArtifact artifact) {
  TestStepArtifact &test_step = *artifact.mutable_test_step_artifact();
  if (test_step.has_test_step_start()) {
    TestStepOutput &step = data.steps[test_step.test_step_id()];
    if (step.start.has_value()) {
      return absl::UnknownError(
          absl::StrFormat("Multiple TestStepStart artifacts for id '%s'",
                          test_step.test_step_id()));
    }

    data.steps[test_step.test_step_id()].start = test_step.test_step_start();
    return absl::OkStatus();
  }

  TestStepOutput &step = data.steps[test_step.test_step_id()];
  if (test_step.has_test_step_end()) {
    if (step.end.has_value()) {
      return absl::UnknownError(absl::StrFormat(
          "Multiple TestStepEnd artifacts for '%s'", test_step.test_step_id()));
    }
    step.end = test_step.test_step_end();
  } else if (test_step.has_log()) {
    step.logs[test_step.log().severity()].emplace_back(test_step.log());
  } else if (test_step.has_error()) {
    step.errors.push_back(test_step.error());
  } else if (test_step.has_diagnosis()) {
    step.diagnoses.push_back(test_step.diagnosis());
  } else if (test_step.has_file()) {
    step.files.push_back(test_step.file());
  } else if (test_step.has_extension()) {
    step.artifact_extensions.push_back(test_step.extension());
  } else if (test_step.has_measurement()) {
    step.measurements.push_back(test_step.measurement());
  } else if (test_step.has_measurement_element()) {
    auto id = test_step.measurement_element().measurement_series_id();
    step.measurement_series[id].measurement_elements.push_back(
        test_step.measurement_element());
  } else if (test_step.has_measurement_series_start()) {
    auto id = test_step.measurement_series_start().measurement_series_id();
    if (step.measurement_series[id].start.has_value()) {
      return absl::UnknownError(absl::StrFormat(
          "Multiple MeasurementSeriesStart artifacts for '%s'", id));
    }
    step.measurement_series[id].start = test_step.measurement_series_start();
  } else if (test_step.has_measurement_series_end()) {
    auto id = test_step.measurement_series_end().measurement_series_id();
    if (step.measurement_series[id].end.has_value()) {
      return absl::UnknownError(absl::StrFormat(
          "Multiple MeasurementSeriesEnd artifacts for '%s'", id));
    }
    step.measurement_series[id].end = test_step.measurement_series_end();
  }
  return absl::OkStatus();
}

}  // namespace

absl::Status AddOutputArtifact(TestRunOutput &output, OutputArtifact artifact) {
  if (artifact.has_test_run_artifact()) {
    RETURN_IF_ERROR(HandleTestRunArtifact(output, std::move(artifact)));
  } else if (artifact.has_test_step_artifact()) {
    RETURN_IF_ERROR(HandleTestStepArtifact(output, std::move(artifact)));
  }
  return absl::OkStatus();
}

}  // namespace ocpdiag::results
