// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/calculator.h"

#include "absl/log/check.h"
#include "absl/synchronization/mutex.h"
#include "ocpdiag/core/results/results.pb.h"

namespace ocpdiag::results {

namespace rpb = ::ocpdiag::results_pb;

rpb::TestResult TestResultCalculator::result() const {
  absl::MutexLock lock(&mutex_);
  return result_;
}

rpb::TestStatus TestResultCalculator::status() const {
  absl::MutexLock lock(&mutex_);
  return status_;
}

void TestResultCalculator::NotifyStartRun() {
  absl::MutexLock lock(&mutex_);
  CHECK(!run_started_) << "Test run already started";
  run_started_ = true;
}

void TestResultCalculator::NotifySkip() {
  absl::MutexLock lock(&mutex_);
  CHECK(!finalized_) << "Test run already finalized";
  if (status_ == ocpdiag::results_pb::TestStatus::UNKNOWN) {
    result_ = ocpdiag::results_pb::NOT_APPLICABLE;
    status_ = ocpdiag::results_pb::SKIPPED;
  }
}

void TestResultCalculator::NotifyError() {
  absl::MutexLock lock(&mutex_);
  CHECK(!finalized_) << "Test run already finalized";
  if (status_ == ocpdiag::results_pb::TestStatus::UNKNOWN) {
    result_ = ocpdiag::results_pb::NOT_APPLICABLE;
    status_ = ocpdiag::results_pb::ERROR;
  }
}

void TestResultCalculator::NotifyFailureDiagnosis() {
  absl::MutexLock lock(&mutex_);
  CHECK(!finalized_) << "Test run already finalized";
  if (result_ == ocpdiag::results_pb::NOT_APPLICABLE &&
      status_ == ocpdiag::results_pb::TestStatus::UNKNOWN) {
    result_ = ocpdiag::results_pb::TestResult::FAIL;
  }
}

void TestResultCalculator::Finalize() {
  absl::MutexLock lock(&mutex_);
  CHECK(!finalized_) << "Test run already finalized";
  finalized_ = true;

  if (run_started_) {
    if (status_ == rpb::TestStatus::UNKNOWN) {
      status_ = rpb::TestStatus::COMPLETE;
      if (result_ == rpb::TestResult::NOT_APPLICABLE) {
        result_ = rpb::TestResult::PASS;
      }
    }
  } else if (status_ != rpb::TestStatus::ERROR) {
    // ERROR status takes highest priority, and so should not be
    // overridden.
    status_ = rpb::TestStatus::SKIPPED;
    result_ = rpb::TestResult::NOT_APPLICABLE;
  }
}

}  // namespace ocpdiag::results
