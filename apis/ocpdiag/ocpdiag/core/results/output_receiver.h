// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_RESULTS_OUTPUT_RECEIVER_H_
#define OCPDIAG_CORE_RESULTS_OUTPUT_RECEIVER_H_

#include <optional>
#include <string>

#include "ocpdiag/core/results/internal/logging.h"
#include "ocpdiag/core/results/output_model.h"
#include "ocpdiag/core/results/recordio_iterator.h"
#include "ocpdiag/core/results/results.pb.h"

namespace ocpdiag::results {

// A helper class for unit tests to consume OCPDiag output artifacts.
//
// The current implementation consumes recordIO output produced by an
// ArtifactWriter.
//
// This class is not thread-safe.
class OutputReceiver {
 public:
  OutputReceiver();

  // We will collect all output artifacts written to this object.
  internal::ArtifactWriter &artifact_writer() const {
    return *artifact_writer_;
  }

  // Returns all the output artifacts in a structured data model. The
  // results are cached after the first call, so you should only call this after
  // the test has run to completion.
  //
  // This will store all the output protobufs in memory at once. If that is a
  // problem, consider using an iterator which holds only one proto in memory
  // at a time.
  const TestRunOutput &model();

  // Returns an iterator to the raw output. Don't use these directly, use them
  // indirectly by using this object as a container in range-based for loops.
  //
  // Example:
  //   for (auto artifact : output_receiver) {...}
  RecordIoIterator<ocpdiag::results_pb::OutputArtifact> begin();
  RecordIoIterator<ocpdiag::results_pb::OutputArtifact> end();

 private:
  const std::string file_path_;
  std::unique_ptr<internal::ArtifactWriter> artifact_writer_;
  std::optional<TestRunOutput> model_;
};

}  // namespace ocpdiag::results

#endif  // OCPDIAG_CORE_RESULTS_OUTPUT_RECEIVER_H_
