// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_LIB_RESULTS_PYTHON_RESULTS_TESTUTILS_H_
#define OCPDIAG_LIB_RESULTS_PYTHON_RESULTS_TESTUTILS_H_

#include <sstream>

#include "ocpdiag/core/results/internal/logging.h"
#include "ocpdiag/core/results/results.h"

namespace ocpdiag {
namespace results {
namespace internal {

// ArtifactWriterStr is wrapper class of ArtifactWriter.
// Used to get the results of Artifact results.
// resultstestpy.clif will wrap this class for python unit testing.
class ArtifactWriterStr {
 public:
  ArtifactWriterStr() : artifacwriter_(-1, &readable_output_) {}
  std::string GetJsonReadableOutput() const { return readable_output_.str(); }

  ArtifactWriter GetArtifactWriter() { return artifacwriter_; }

  void Reset() { readable_output_.str(""); }

  void RegisterHwId(absl::string_view i) { artifacwriter_.RegisterHwId(i); }
  void RegisterSwId(absl::string_view i) { artifacwriter_.RegisterSwId(i); }

 private:
  std::stringstream readable_output_;
  ArtifactWriter artifacwriter_;
};

// ResultsTest provide a interface for Create TestRun and TestStep.
// resultstestpy.clif will wrap this class for python unit testing.
class ResultsTest {
 public:
  ResultsTest()
      : artifact_writer_(
            std::make_shared<ArtifactWriter>(-1, &readable_output_)) {}
  static constexpr char kStepIdDefault[] = "0";
  TestRun InitializeTestRun(std::string name) {
    return TestRun(name, artifact_writer_);
  }

  TestStep CreateTestStep(TestRun* runner, std::string name,
                          std::string customId = kStepIdDefault) {
    return TestStep(runner, std::move(customId), std::move(name),
                    artifact_writer_);
  }
  MeasurementSeries CreateMeasurementSeries(
      const ocpdiag::results_pb::MeasurementInfo& info) {
    return MeasurementSeries(nullptr, kStepIdDefault, "series_id",
                             artifact_writer_, info);
  }
  std::string GetJsonReadableOutput() const { return readable_output_.str(); }

  void Reset() { readable_output_.str(""); }

  void RegisterHwId(absl::string_view i) { artifact_writer_->RegisterHwId(i); }
  void RegisterSwId(absl::string_view i) { artifact_writer_->RegisterSwId(i); }

 private:
  std::stringstream readable_output_;
  std::shared_ptr<ArtifactWriter> artifact_writer_;
};

}  // namespace internal
}  // namespace results
}  // namespace ocpdiag

#endif  // OCPDIAG_LIB_RESULTS_PYTHON_RESULTS_TESTUTILS_H_
