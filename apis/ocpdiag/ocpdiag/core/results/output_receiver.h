// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_RESULTS_OUTPUT_RECEIVER_H_
#define OCPDIAG_CORE_RESULTS_OUTPUT_RECEIVER_H_

#include <string>

#include "absl/flags/reflection.h"
#include "ocpdiag/core/results/output_model.h"
#include "ocpdiag/core/results/results.pb.h"

namespace ocpdiag::results {

// A helper class for unit tests to consume OCPDiag output artifacts.
//
// The current implementation consumes recordIO output produced by the TestRun
// library, but future implementations may decide to do this differently without
// needing to change the unit testing API.
class OutputReceiver {
 public:
  using OutputCallback =
      std::function<bool(ocpdiag::results_pb::OutputArtifact)>;

  // Begins capturing OCPDiag results output, or crashes with a stack trace if
  // it's unable to for some reason.
  OutputReceiver();
  ~OutputReceiver() { Close(); }

  // Returns the output that was received. After calling this the first time,
  // the results never change, so only call this after the OCPDiag test has
  // finished.
  //
  // This will store all the output protobufs in memory at once. If that is a
  // problem, consider Iterate() to only hold one proto in memory at a time.
  const TestRunOutput &model();

  // Iterates over all results, executing the callback on each one.
  // This method avoids storing all output in memory at once.
  void Iterate(const OutputCallback &callback);

  // Closes down the receiver. This does not need to be called explicitly,
  // except for in Python wrappers.
  void Close();

 private:
  // Consumes the test run output and populates a new model.
  TestRunOutput ConsumeOutputOrDie();

  std::unique_ptr<absl::FlagSaver> flag_saver_;
  const std::string filename_;
  std::optional<TestRunOutput> output_;
};

}  // namespace ocpdiag::results

#endif  // OCPDIAG_CORE_RESULTS_OUTPUT_RECEIVER_H_
