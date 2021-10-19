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

#include "meltan/core/examples/meltandiag/my_meltan_diagnostic.h"

#include <stdlib.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/struct.pb.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "meltan/lib/results/results.h"
#include "meltan/lib/results/results.pb.h"
#include "meltan/lib/testing/mock_results.h"
#include "meltan/lib/testing/proto_matchers.h"

namespace {

using ::meltan::results::HwRecord;
using ::meltan::results::TestRun;
using ::meltan::results::TestStep;
using ::meltan::results::testonly::FakeResultApi;
using ::meltan::results::testonly::MockMeasurementSeries;
using ::meltan::results::testonly::MockResultApi;
using ::meltan::results::testonly::MockTestRun;
using ::meltan::results::testonly::MockTestStep;

using ::testing::_;
using ::meltan::testing::EqualsProto;
using ::testing::Return;

namespace rpb = ::meltan::results_pb;

//

// Test fixture that manages the lifetime of MockResultApi, since a MeltanDiag
// does not own an injected API.
class MyMeltanDiagnosticTest : public ::testing::Test {
 protected:
  MyMeltanDiagnosticTest() : diag_(MyMeltanDiagnostic(&api_)) {}

  MockResultApi api_;
  MyMeltanDiagnostic diag_;
};

// How to mock a TestRun
TEST_F(MyMeltanDiagnosticTest, takesTestRunTest) {
  MockTestRun runner("mock test");
  // tell MockTestRun to default to real implementation.
  runner.DelegateToReal();
  // place an expectation on one of its methods.
  EXPECT_CALL(runner, LogInfo(absl::string_view("log info"))).Times(1);
  // stub the End() method to simulate failure.
  ON_CALL(runner, End()).WillByDefault(Return(rpb::TestResult::FAIL));

  // Call code under test
  EXPECT_EQ(diag_.TakesTestRun(&runner), rpb::TestResult::FAIL);
}

// How to mock a TestStep and fake HwRecords
TEST_F(MyMeltanDiagnosticTest, takesTestStepTest) {
  MockTestStep step("mock step");
  // Fake some records. For unit tests, we don't really care if they're
  // registered or not.
  std::vector<HwRecord> records = {HwRecord(), HwRecord(), HwRecord()};
  // Expect one diag per record
  EXPECT_CALL(step, AddDiagnosis(rpb::Diagnosis::PASS, "pass diag", "msg", _))
      .Times(records.size());
  diag_.TakesTestStep(&step, absl::MakeSpan(records));
}

// How to mock a MeasurementSeries
TEST_F(MyMeltanDiagnosticTest, takesMeasurementSeriesTest) {
  MockMeasurementSeries ms;
  EXPECT_CALL(
      ms, AddElementWithRange(EqualsProto(google::protobuf::Value()),
                              EqualsProto(rpb::MeasurementElement::Range())));
  diag_.TakesMeasurementSeries(&ms);
}

// How to stub ResultApi methods to return mock objects anywhere in production
// code, then write expectations on those mocks.
TEST_F(MyMeltanDiagnosticTest, stubTest) {
  // Create some mocks to be returned from stubbed methods.
  auto mock_runner = absl::make_unique<MockTestRun>("mock_runner");
  auto mock_step = absl::make_unique<MockTestStep>("mock_step");
  auto mock_series = absl::make_unique<MockMeasurementSeries>();

  // Stub the MockResultApi methods.
  // DO NOT USE ::testing::Return unless you've already set expectations on
  // mocks. Instead use lambda because the std::move will happen inline with
  // code under test, rather than here.
  EXPECT_CALL(api_, InitializeTestRun(_)).WillOnce([&](std::string s) {
    return std::move(mock_runner);
  });
  EXPECT_CALL(api_, BeginTestStep(_, "myStep"))
      .WillOnce(
          [&](TestRun* t, std::string s) { return std::move(mock_step); });
  EXPECT_CALL(api_, BeginMeasurementSeries(_, _, _))
      .WillOnce([&](TestStep* t, const HwRecord& hw,
                    meltan::results_pb::MeasurementInfo i) {
        return std::move(mock_series);
      });

  // Set expectations. mock ptrs are still valid because std::move is deferred
  // until InitializeTestRun is actually called in the code under test.
  EXPECT_CALL(*mock_runner, StartAndRegisterInfos(_, _)).Times(1);
  EXPECT_CALL(*mock_step, LogInfo(absl::string_view("Step began"))).Times(1);
  EXPECT_CALL(*mock_series, AddElementWithRange(_, _)).Times(1);

  // Finally, call the code to be tested
  EXPECT_EQ(diag_.ExecuteTest(), EXIT_SUCCESS);

  // WARNING: if the above stubbed methods are called in ExecuteTest(), then the
  // mock ptrs have been moved-from and are thus invalid at this point!
}

// Test injecting fail scenario into API
TEST_F(MyMeltanDiagnosticTest, failTestRunInit) {
  // Tell MockResultApi to delegate method calls to a fake by default.
  api_.DelegateToFake();
  // Stub one method so the api fails to initialize a TestRun
  EXPECT_CALL(api_, InitializeTestRun(_)).WillOnce([](std::string) {
    return absl::PermissionDeniedError(
        "you don't have permission to write to '/tmp', that's a folder");
  });
  EXPECT_EQ(diag_.ExecuteTest(), EXIT_FAILURE);
}

// Basic integration test.
// Use FakeResultApi so gMock does not flood test log with "uninteresting call".
TEST(Integration, ExecuteTest_happy_path) {
  FakeResultApi api;
  MyMeltanDiagnostic diag(&api);
  EXPECT_EQ(diag.ExecuteTest(), EXIT_SUCCESS);
}

}  // namespace
