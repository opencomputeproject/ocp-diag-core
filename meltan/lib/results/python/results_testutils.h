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

#ifndef MELTAN_LIB_RESULTS_PYTHON_RESULTS_TESTUTILS_H_
#define MELTAN_LIB_RESULTS_PYTHON_RESULTS_TESTUTILS_H_

#include <sstream>

#include "meltan/lib/results/internal/logging.h"
#include "meltan/lib/results/results.h"

namespace meltan {
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
  ResultsTest() : artifact_writer_(-1, &readable_output_) {}
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
      const meltan::results_pb::MeasurementInfo& info) {
    return MeasurementSeries(nullptr, kStepIdDefault, "series_id",
                             artifact_writer_, info);
  }
  std::string GetJsonReadableOutput() const { return readable_output_.str(); }

  void Reset() { readable_output_.str(""); }

  void RegisterHwId(absl::string_view i) { artifact_writer_.RegisterHwId(i); }
  void RegisterSwId(absl::string_view i) { artifact_writer_.RegisterSwId(i); }

 private:
  std::stringstream readable_output_;
  ArtifactWriter artifact_writer_;
};

}  // namespace internal
}  // namespace results
}  // namespace meltan

#endif  // MELTAN_LIB_RESULTS_PYTHON_RESULTS_TESTUTILS_H_
