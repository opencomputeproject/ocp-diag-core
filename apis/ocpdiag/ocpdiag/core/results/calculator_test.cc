// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/calculator.h"

#include "gtest/gtest.h"
#include "ocpdiag/core/results/results.pb.h"

namespace ocpdiag::results {
namespace {

namespace rpb = ::ocpdiag::results_pb;

TEST(TestResultCalculatorTest, Passing) {
  TestResultCalculator calculator;
  calculator.NotifyStartRun();
  calculator.Finalize();
  EXPECT_EQ(calculator.result(), rpb::PASS);
  EXPECT_EQ(calculator.status(), rpb::COMPLETE);
}

TEST(TestResultCalculatorTest, SkippedNotStarted) {
  TestResultCalculator calculator;
  calculator.Finalize();
  EXPECT_EQ(calculator.result(), rpb::NOT_APPLICABLE);
  EXPECT_EQ(calculator.status(), rpb::SKIPPED);
}

TEST(TestResultCalculatorTest, SkippedIntentionally) {
  TestResultCalculator calculator;
  calculator.NotifyStartRun();
  calculator.NotifySkip();
  calculator.Finalize();
  EXPECT_EQ(calculator.result(), rpb::NOT_APPLICABLE);
  EXPECT_EQ(calculator.status(), rpb::SKIPPED);
}

TEST(TestResultCalculatorTest, Error) {
  TestResultCalculator calculator;
  calculator.NotifyStartRun();
  calculator.NotifyError();
  calculator.Finalize();
  EXPECT_EQ(calculator.result(), rpb::NOT_APPLICABLE);
  EXPECT_EQ(calculator.status(), rpb::ERROR);
}

TEST(TestResultCalculatorTest, ErrorBeforeStart) {
  TestResultCalculator calculator;
  calculator.NotifyError();
  calculator.NotifyStartRun();
  calculator.Finalize();
  EXPECT_EQ(calculator.result(), rpb::NOT_APPLICABLE);
  EXPECT_EQ(calculator.status(), rpb::ERROR);
}

TEST(TestResultCalculatorTest, SkipDoesNotOverrideError) {
  TestResultCalculator calculator;
  calculator.NotifyStartRun();
  calculator.NotifyError();
  calculator.NotifySkip();
  calculator.Finalize();
  EXPECT_EQ(calculator.result(), rpb::NOT_APPLICABLE);
  EXPECT_EQ(calculator.status(), rpb::ERROR);
}

TEST(TestResultCalculatorTest, Failing) {
  TestResultCalculator calculator;
  calculator.NotifyStartRun();
  calculator.NotifyFailureDiagnosis();
  calculator.Finalize();
  EXPECT_EQ(calculator.result(), rpb::FAIL);
  EXPECT_EQ(calculator.status(), rpb::COMPLETE);
}

TEST(TestResultCalculatorTest, ErrorOverridesFail) {
  TestResultCalculator calculator;
  calculator.NotifyStartRun();
  calculator.NotifyFailureDiagnosis();
  calculator.NotifyError();
  calculator.Finalize();
  EXPECT_EQ(calculator.result(), rpb::NOT_APPLICABLE);
  EXPECT_EQ(calculator.status(), rpb::ERROR);
}

}  // namespace
}  // namespace ocpdiag::results
