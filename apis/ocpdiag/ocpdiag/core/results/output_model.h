// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_RESULTS_OUTPUT_MODEL_H_
#define OCPDIAG_CORE_RESULTS_OUTPUT_MODEL_H_

#include <optional>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "ocpdiag/core/results/results.pb.h"

namespace ocpdiag::results {

struct TestStepOutput;
struct MeasurementSeriesOutput;

// Organizes all the output artifacts of a OCPDiag diag test run.
struct TestRunOutput {
  // We use this type to organize log protos, indexed by severity.
  using LogsBySeverity =
      absl::flat_hash_map<ocpdiag::results_pb::Log::Severity,
                          std::vector<ocpdiag::results_pb::Log>>;

  using TestStepMap = absl::flat_hash_map<std::string, TestStepOutput>;

  // The test run start artifact.
  std::optional<ocpdiag::results_pb::TestRunStart> start;

  // The test run end artifact.
  std::optional<ocpdiag::results_pb::TestRunEnd> end;

  // Any test run logs, indexed by severity.
  LogsBySeverity logs;

  // Any tags produced by the test run.
  std::vector<ocpdiag::results_pb::Tag> tags;

  // Any errors produced by the test run.
  std::vector<ocpdiag::results_pb::Error> errors;

  // Test steps are indexed by ID.
  TestStepMap steps;
};

// Adds a single output artifact to the object.
absl::Status AddOutputArtifact(
    TestRunOutput &output,
    ocpdiag::results_pb::OutputArtifact artifact);

// Organizes all the output artifacts of a OCPDiag test step.
struct TestStepOutput {
  // A map of measurement series ID to the series output struct.
  using MeasurementSeriesMap =
      absl::flat_hash_map<std::string, MeasurementSeriesOutput>;

  // The test step start artifact.
  std::optional<ocpdiag::results_pb::TestStepStart> start;

  // The test step end artifact.
  std::optional<ocpdiag::results_pb::TestStepEnd> end;

  // Any test step logs, indexed by severity.
  TestRunOutput::LogsBySeverity logs;

  // Any errors produced by the test step.
  std::vector<ocpdiag::results_pb::Error> errors;

  // Any files uploaded by the test step.
  std::vector<ocpdiag::results_pb::File> files;

  // Any artifact extensions added to the test step.
  std::vector<ocpdiag::results_pb::ArtifactExtension>
      artifact_extensions;

  // Any measurement series created by the test step, indexed by series ID.
  MeasurementSeriesMap measurement_series;

  // The measurements.
  std::vector<ocpdiag::results_pb::Measurement> measurements;

  // The test step diagnoses (there may be more than one).
  std::vector<ocpdiag::results_pb::Diagnosis> diagnoses;
};

// Organizes all the output artifacts of a measurement series.
struct MeasurementSeriesOutput {
  // The measurement series start artifact.
  std::optional<ocpdiag::results_pb::MeasurementSeriesStart> start;

  // The measurement series end artifact.
  std::optional<ocpdiag::results_pb::MeasurementSeriesEnd> end;

  // The measurement elements.
  std::vector<ocpdiag::results_pb::MeasurementElement>
      measurement_elements;
};

}  // namespace ocpdiag::results

#endif  // OCPDIAG_CORE_RESULTS_OUTPUT_MODEL_H_
