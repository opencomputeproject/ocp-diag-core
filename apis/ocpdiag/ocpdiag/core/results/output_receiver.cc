// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/output_receiver.h"

#include <cstdlib>
#include <filesystem>  //
#include <memory>
#include <optional>

#include "absl/log/check.h"
#include "absl/strings/str_format.h"
#include "ocpdiag/core/compat/status_macros.h"
#include "ocpdiag/core/results/recordio_iterator.h"
#include "ocpdiag/core/results/results.pb.h"
#include "ocpdiag/core/results/results_model.pb.h"

namespace ocpdiag::results {
namespace {
using ::ocpdiag::results_pb::Log;
using ::ocpdiag::results_pb::OutputArtifact;
using ::ocpdiag::results_pb::TestRunArtifact;
using ::ocpdiag::results_pb::TestStepArtifact;

absl::Status HandleLog(ocpdiag_results_pb::LogsModel &model, const Log &log) {
  Log *model_log = nullptr;
  switch (log.severity()) {
    case Log::DEBUG:
      model_log = model.add_debug();
      break;
    case Log::INFO:
      model_log = model.add_info();
      break;
    case Log::WARNING:
      model_log = model.add_warning();
      break;
    case Log::ERROR:
      model_log = model.add_error();
      break;
    case Log::FATAL:
      model_log = model.add_fatal();
      break;
    default:
      return absl::UnknownError("Unexpected log severity");
  }
  *model_log = log;
  return absl::OkStatus();
}

absl::Status HandleTestRunArtifact(ocpdiag_results_pb::TestRunModel &model,
                                   const OutputArtifact &artifact) {
  const TestRunArtifact &test_run = artifact.test_run_artifact();
  if (test_run.has_test_run_start()) {
    if (!model.start().name().empty())
      return absl::UnknownError("Multiple TestRunStart artifacts");
    *model.mutable_start() = test_run.test_run_start();
  } else if (test_run.has_test_run_end()) {
    if (!model.end().name().empty())
      return absl::UnknownError("Multiple TestRunEnd artifacts");
    *model.mutable_end() = test_run.test_run_end();
  } else if (test_run.has_log()) {
    RETURN_IF_ERROR(HandleLog(*model.mutable_logs(), test_run.log()));
  } else if (test_run.has_tag()) {
    *model.add_tags() = test_run.tag();
  } else if (test_run.has_error()) {
    *model.add_errors() = test_run.error();
  }
  return absl::OkStatus();
}

absl::Status HandleTestStepArtifact(ocpdiag_results_pb::TestRunModel &model,
                                    const OutputArtifact &artifact) {
  const TestStepArtifact &test_step = artifact.test_step_artifact();
  if (test_step.has_test_step_start()) {
    ocpdiag_results_pb::TestStepModel &step =
        (*model.mutable_test_steps())[test_step.test_step_id()];
    if (!step.start().name().empty()) {
      return absl::UnknownError(
          absl::StrFormat("Multiple TestStepStart artifacts for id '%s'",
                          test_step.test_step_id()));
    }
    *step.mutable_start() = test_step.test_step_start();
    return absl::OkStatus();
  }

  ocpdiag_results_pb::TestStepModel &step =
      (*model.mutable_test_steps())[test_step.test_step_id()];
  if (test_step.has_test_step_end()) {
    if (!step.end().name().empty()) {
      return absl::UnknownError(absl::StrFormat(
          "Multiple TestStepEnd artifacts for '%s'", test_step.test_step_id()));
    }
    *step.mutable_end() = test_step.test_step_end();
  } else if (test_step.has_log()) {
    RETURN_IF_ERROR(HandleLog(*step.mutable_logs(), test_step.log()));
  } else if (test_step.has_error()) {
    *step.add_errors() = test_step.error();
  } else if (test_step.has_diagnosis()) {
    *step.add_diagnoses() = test_step.diagnosis();
  } else if (test_step.has_file()) {
    *step.add_files() = test_step.file();
  } else if (test_step.has_extension()) {
    *step.add_artifact_extensions() = test_step.extension();
  } else if (test_step.has_measurement()) {
    *step.add_measurements() = test_step.measurement();
  } else if (test_step.has_measurement_element()) {
    auto id = test_step.measurement_element().measurement_series_id();
    *(*step.mutable_measurement_series())[id].add_measurement_elements() =
        test_step.measurement_element();
  } else if (test_step.has_measurement_series_start()) {
    auto id = test_step.measurement_series_start().measurement_series_id();
    auto &start = *(*step.mutable_measurement_series())[id].mutable_start();
    if (!start.measurement_series_id().empty()) {
      return absl::UnknownError(absl::StrFormat(
          "Multiple MeasurementSeriesStart artifacts for '%s'", id));
    }
    start = test_step.measurement_series_start();
  } else if (test_step.has_measurement_series_end()) {
    auto id = test_step.measurement_series_end().measurement_series_id();
    auto &end = *(*step.mutable_measurement_series())[id].mutable_end();
    if (!end.measurement_series_id().empty()) {
      return absl::UnknownError(absl::StrFormat(
          "Multiple MeasurementSeriesEnd artifacts for '%s'", id));
    }
    end = test_step.measurement_series_end();
  }
  return absl::OkStatus();
}

}  // namespace

absl::Status AddOutputArtifact(ocpdiag_results_pb::TestRunModel &model,
                               OutputArtifact artifact) {
  if (artifact.has_test_run_artifact()) {
    RETURN_IF_ERROR(HandleTestRunArtifact(model, std::move(artifact)));
  } else if (artifact.has_test_step_artifact()) {
    RETURN_IF_ERROR(HandleTestStepArtifact(model, std::move(artifact)));
  }
  return absl::OkStatus();
}

OutputReceiver::OutputReceiver()
    : file_path_([] {
        // Try several options for a temporary filename until a suitable
        // one is found. Note that the tempdir may be different depending on
        // environment.
        std::string path = std::getenv("TEST_TMPDIR");
        if (path.empty()) path = std::getenv("TMPDIR");
        if (path.empty()) path = std::filesystem::temp_directory_path();
        path += "/ocpdiag_output_receiver_tempfile_XXXXXX";
        CHECK_NE(mkstemp(path.data()), -1) << "Cannot make temp file path";

        // Prepare the output file to receive test data.
        if (std::filesystem::exists(path))
          CHECK(std::filesystem::remove(path)) << "Cannot remove temp file";
        return path;
      }()),
      artifact_writer_([&] {
        // Create an artifact writer that outputs to a file, as well as stdout
        // for easier examination during unit tests.
        absl::StatusOr<int> fd = internal::OpenAndGetDescriptor(file_path_);
        CHECK_OK(fd.status()) << "Cannot open file";
        bool log_to_stdout = true;
        return std::make_unique<internal::ArtifactWriter>(*fd, log_to_stdout);
      }()) {}

const ocpdiag_results_pb::TestRunModel &OutputReceiver::model() {
  // Populate the test run output model the first time this is called.
  if (!model_.has_value()) {
    model_ = ocpdiag_results_pb::TestRunModel{};
    for (OutputArtifact artifact : *this) {
      CHECK_OK(AddOutputArtifact(*model_, std::move(artifact)));
    }
  }
  return *model_;
}

OutputReceiver::const_iterator OutputReceiver::begin() const {
  return RecordIoIterator<OutputArtifact>(file_path_);
}

OutputReceiver::const_iterator OutputReceiver::end() const {
  return RecordIoIterator<OutputArtifact>();
}

}  // namespace ocpdiag::results
