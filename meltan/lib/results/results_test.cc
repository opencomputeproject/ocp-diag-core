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

#include "meltan/lib/results/results.h"

#include <unistd.h>

#include <future>  //
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/empty.pb.h"
#include "google/protobuf/struct.pb.h"
#include "google/protobuf/timestamp.pb.h"
#include "google/protobuf/util/json_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "meltan/core/compat/status_converters.h"
#include "meltan/lib/results/internal/logging.h"
#include "meltan/lib/results/internal/test_utils.h"
#include "meltan/lib/results/results.pb.h"
#include "meltan/lib/testing/mock_results.h"
#include "meltan/lib/testing/parse_text_proto.h"
#include "meltan/lib/testing/proto_matchers.h"
#include "meltan/lib/testing/status_matchers.h"

namespace meltan {
namespace results {
namespace internal {

using internal::ArtifactWriter;
using internal::Now;
using internal::TestFile;
using ::meltan::testing::EqualsProto;
using ::meltan::testing::ParseTextProtoOrDie;
using ::meltan::testing::Partially;
using ::meltan::testing::StatusIs;
using ::testing::_;

namespace rpb = ::meltan::results_pb;

constexpr const absl::string_view kSympUnregHw = "unregistered-hardware-info";
constexpr const absl::string_view kSympUnregSw = "unregistered-software-info";

constexpr char kStepIdDefault[] = "0";
constexpr char kHwRegistered[] = R"pb(
  hardware_info_id: "%s"
  arena: "arena"
  name: "name"
  fru_location {
    devpath: "fru devpath"
    odata_id: "fru odata_id"
    blockpath: "fru blockpath"
    serial_number: "fru serial_number"
  }
  part_number: "part_number"
  manufacturer: "manufacturer"
  mfg_part_number: "mfg_part_number"
  part_type: "part_type"
  component_location {
    devpath: "component devpath"
    odata_id: "component odata_id"
    blockpath: "component blockpath"
  }
)pb";

constexpr char kHwNotRegistered[] = R"pb(
  hardware_info_id: "not registered"
)pb";

constexpr char kSwRegistered[] =
    R"pb(
  software_info_id: "%s"
  arena: "arena"
  name: "name"
  version: "sw version"
    )pb";

constexpr char kSwNotRegistered[] = R"pb(
  software_info_id: "not registered"
)pb";

constexpr char kMeasurementInfo[] =
    R"pb(
  name: "measurement info" unit: "unit" hardware_info_id: "%s"
    )pb";

// Convert JSONL to OutputArtifact proto, or fail test
void toProtoOrFail(std::string s, rpb::OutputArtifact& out) {
  ASSERT_OK(
      AsAbslStatus(google::protobuf::util::JsonStringToMessage(std::move(s), &out)));
}

// Test fixture for shared, static helper methods and members
// Class instead of free functions to simplify friending
//
class ResultsTest : public ::testing::Test {
 public:
  static TestRun GenerateTestRun(std::string name, ArtifactWriter&& w) {
    return TestRun(name, std::forward<ArtifactWriter>(w));
  }

  static HwRecord GenerateHwRecord(rpb::HardwareInfo info) {
    DutInfo di("hostname");
    return di.AddHardware(info);
  }

  static SwRecord GenerateSwRecord(rpb::SoftwareInfo info) {
    DutInfo di("hostname");
    return di.AddSoftware(info);
  }

  static TestStep GenerateTestStep(TestRun* parent, std::string name,
                                   internal::ArtifactWriter&& writer,
                                   std::string customId = kStepIdDefault) {
    return TestStep(parent, customId, std::move(name),
                    std::forward<ArtifactWriter>(writer));
  }

  static MeasurementSeries GenerateMeasurementSeries(
      internal::ArtifactWriter writer) {
    rpb::HardwareInfo hwinfo = ParseTextProtoOrDie(kHwRegistered);
    HwRecord hw = GenerateHwRecord(hwinfo);
    rpb::MeasurementInfo minfo = ParseTextProtoOrDie(kMeasurementInfo);
    return MeasurementSeries(nullptr, kStepIdDefault, "series_id",
                             std::move(writer), minfo);
  }

  static HwRecord hwReg_;
  static HwRecord hwNotReg_;

 protected:
  ResultsTest() = default;
  static void SetUpTestSuite() {}
  void SetUp() {}
  ~ResultsTest() override = default;
};

// Test fixture for TestRun
class TestRunTest : public ::testing::Test {
 protected:
  TestRunTest()
      : test_(ResultsTest::GenerateTestRun(
            "TestRunTest",
            internal::ArtifactWriter(dup(tmpfile_.descriptor), &json_out_))) {}
  void SetUp() { json_out_.clear(); }
  ~TestRunTest() override = default;

  std::stringstream json_out_;
  TestFile tmpfile_;
  rpb::OutputArtifact got_;
  TestRun test_;
};

TEST_F(TestRunTest, InitOnlyOnce) {
  absl::StatusOr<TestRun> test = TestRun::Init("orig");
  ASSERT_OK(test);
  absl::StatusOr<TestRun> dupe = TestRun::Init("dupe");
  ASSERT_THAT(dupe, StatusIs(absl::StatusCode::kAlreadyExists));
}

TEST_F(TestRunTest, StartAndRegisterInfos) {
  ASSERT_FALSE(test_.Started());
  DutInfo di("host");
  DutInfo di2("host2");
  HwRecord hw = di.AddHardware(ParseTextProtoOrDie(kHwRegistered));
  SwRecord sw = di.AddSoftware(ParseTextProtoOrDie(kSwRegistered));
  di.AddPlatformInfo("plugin:something");

  std::vector<meltan::results::DutInfo> dutinfos;
  dutinfos.emplace_back(std::move(di));
  dutinfos.emplace_back(std::move(di2));
  google::protobuf::Empty params;
  test_.StartAndRegisterInfos(dutinfos, params);
  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(
      R"pb(
        test_run_artifact {
          test_run_start {
            name: "TestRunTest"
            parameters {
              type_url: "type.googleapis.com/google.protobuf.Empty"
              value: "%s"
            }
            dut_info {
              hostname: "host"
              hardware_components { %s }
              software_infos { %s }
              platform_info { info: "plugin:something" }
            }
            dut_info { hostname: "host2" }
          }
        }
        sequence_number: 0
      )pb",
      params.SerializeAsString(),
      absl::StrFormat(kHwRegistered, hw.Data().hardware_info_id()),
      absl::StrFormat(kSwRegistered, sw.Data().software_info_id()));
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
  EXPECT_NE(got_.test_run_artifact().test_run_start().version(), "");
  ASSERT_TRUE(test_.Started());
}

TEST_F(TestRunTest, EndBeforeStart) {
  ASSERT_FALSE(test_.Ended());
  rpb::TestResult result = test_.End();
  // Expect NOT_APPLICABLE since test was never started
  ASSERT_EQ(result, rpb::TestResult::NOT_APPLICABLE);
}

TEST_F(TestRunTest, EndAfterStart) {
  ASSERT_FALSE(test_.Ended());
  test_.StartAndRegisterInfos({});
  rpb::TestResult result = test_.End();
  ASSERT_TRUE(test_.Ended());
  ASSERT_EQ(result, rpb::TestResult::PASS);
  ASSERT_EQ(test_.Status(), rpb::TestStatus::COMPLETE);

  std::string line;
  std::getline(json_out_, line);  // discard TestRunStart artifact
  std::getline(json_out_, line);
  toProtoOrFail(line, got_);
  std::string want = absl::StrFormat(R"pb(
    test_run_artifact {
      test_run_end { name: "TestRunTest" status: COMPLETE result: PASS }
    }
  )pb");
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
  test_.GetWriter().Close();
}

// Even if TestRun::End() called multiple times, expect only 1 artifact
TEST_F(TestRunTest, EndTwice) {
  test_.End();
  test_.End();
  test_.GetWriter().Close();
}

// Expect default NOT_APPLICABLE:UNKNOWN
TEST_F(TestRunTest, ResultCalc_Defaults) {
  EXPECT_EQ(rpb::TestStatus_Name(test_.Status()),
            rpb::TestStatus_Name(rpb::TestStatus::UNKNOWN));
  EXPECT_EQ(rpb::TestResult_Name(test_.Result()),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
}

// Expect NOT_APPLICABLE:SKIPPED if skipped and no errors emitted
TEST_F(TestRunTest, ResultCalc_Skip) {
  rpb::TestResult result = test_.Skip();
  EXPECT_EQ(rpb::TestStatus_Name(test_.Status()),
            rpb::TestStatus_Name(rpb::TestStatus::SKIPPED));
  EXPECT_EQ(rpb::TestResult_Name(test_.Result()),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
  EXPECT_EQ(rpb::TestResult_Name(result),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
}

// Expect NOT_APPLICABLE:ERROR even if skip called after error
TEST_F(TestRunTest, ResultCalc_Error_then_Skip) {
  test_.AddError("", "");
  rpb::TestResult result = test_.Skip();
  EXPECT_EQ(rpb::TestStatus_Name(test_.Status()),
            rpb::TestStatus_Name(rpb::TestStatus::ERROR));
  EXPECT_EQ(rpb::TestResult_Name(test_.Result()),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
  EXPECT_EQ(rpb::TestResult_Name(result),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
}

// Expect PASS:COMPLETE if started and no diags/errors emitted
TEST_F(TestRunTest, ResultCalc_Pass) {
  test_.StartAndRegisterInfos({});
  rpb::TestResult result = test_.End();
  EXPECT_EQ(rpb::TestStatus_Name(test_.Status()),
            rpb::TestStatus_Name(rpb::TestStatus::COMPLETE));
  EXPECT_EQ(rpb::TestResult_Name(test_.Result()),
            rpb::TestResult_Name(rpb::TestResult::PASS));
  EXPECT_EQ(rpb::TestResult_Name(result),
            rpb::TestResult_Name(rpb::TestResult::PASS));
}

// Expect FAIL:COMPLETE if started and fail diag emitted
TEST_F(TestRunTest, ResultCalc_AddFailDiag) {
  test_.StartAndRegisterInfos({});
  TestStep step = ResultsTest::GenerateTestStep(
      &test_, "", internal::ArtifactWriter(-1, nullptr));
  step.AddDiagnosis(rpb::Diagnosis::FAIL, "", "", {});
  rpb::TestResult result = test_.End();
  EXPECT_EQ(rpb::TestStatus_Name(test_.Status()),
            rpb::TestStatus_Name(rpb::TestStatus::COMPLETE));
  EXPECT_EQ(rpb::TestResult_Name(test_.Result()),
            rpb::TestResult_Name(rpb::TestResult::FAIL));
  EXPECT_EQ(rpb::TestResult_Name(result),
            rpb::TestResult_Name(rpb::TestResult::FAIL));
}

// Expect NOT_APPLICABLE:ERROR if error artifact emitted from TestRun
TEST_F(TestRunTest, ResultCalc_TestRun_AddError) {
  test_.AddError("", "");
  rpb::TestResult result = test_.End();
  EXPECT_EQ(rpb::TestStatus_Name(test_.Status()),
            rpb::TestStatus_Name(rpb::TestStatus::ERROR));
  EXPECT_EQ(rpb::TestResult_Name(test_.Result()),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
  EXPECT_EQ(rpb::TestResult_Name(result),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
}

// Expect NOT_APPLICABLE:ERROR if error artifact emitted from TestStep
TEST_F(TestRunTest, ResultCalc_TestStep_AddError) {
  test_.StartAndRegisterInfos({});
  TestStep step = ResultsTest::GenerateTestStep(
      &test_, "", internal::ArtifactWriter(-1, nullptr));
  step.AddError("", "", {});
  rpb::TestResult result = test_.End();
  EXPECT_EQ(rpb::TestStatus_Name(test_.Status()),
            rpb::TestStatus_Name(rpb::TestStatus::ERROR));
  EXPECT_EQ(rpb::TestResult_Name(test_.Result()),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
  EXPECT_EQ(rpb::TestResult_Name(result),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
}

TEST_F(TestRunTest, AddError) {
  SwRecord swrec =
      ResultsTest::GenerateSwRecord(ParseTextProtoOrDie(kSwRegistered));
  test_.AddError("symptom", "msg");

  toProtoOrFail(json_out_.str(), got_);
  // Compare to what we expect (ignoring timestamp)
  std::string want = absl::StrFormat(R"pb(
    test_run_artifact { error { symptom: "symptom" msg: "msg" } }
  )pb");
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

TEST_F(TestRunTest, AddTag) {
  test_.AddTag("Guten Tag");
  toProtoOrFail(json_out_.str(), got_);
  // Compare to what we expect (ignoring timestamp)
  std::string want = absl::StrFormat(R"pb(
    test_run_artifact { tag { tag: "Guten Tag" } }
  )pb");
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

TEST_F(TestRunTest, LogDebug) {
  test_.LogDebug("my house has termites, please debug it");
  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(R"pb(
    test_run_artifact {
      log { severity: DEBUG text: "my house has termites, please debug it" }
    }
  )pb");
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

TEST_F(TestRunTest, LogInfo) {
  test_.LogInfo("Here, have an info");
  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(R"pb(
    test_run_artifact { log { severity: INFO text: "Here, have an info" } }
  )pb");
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

TEST_F(TestRunTest, LogWarn) {
  test_.LogWarn("this test is brand new, never warn");
  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(R"pb(
    test_run_artifact {
      log { severity: WARNING text: "this test is brand new, never warn" }
    }
  )pb");
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

TEST_F(TestRunTest, LogError) {
  test_.LogError("to err is human");
  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(R"pb(
    test_run_artifact { log { severity: ERROR text: "to err is human" } }
  )pb");
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

TEST_F(TestRunTest, LogFatal) {
  test_.LogFatal("What's my destiny? Fatal determine that");
  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(R"pb(
    test_run_artifact {
      log { severity: FATAL text: "What's my destiny? Fatal determine that" }
    }
  )pb");
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
  //
}

// Test fixture for TestStep
class TestStepTest : public ::testing::Test {
 protected:
  TestStepTest()
      : parent_(ResultsTest::GenerateTestRun(
            "TestRun", internal::ArtifactWriter(-1, nullptr))),
        step_(ResultsTest::GenerateTestStep(
            &parent_, "TestStepTest",
            internal::ArtifactWriter(dup(tmpfile_.descriptor), &json_out_))) {}
  void SetUp() { json_out_.clear(); }
  ~TestStepTest() override = default;

  TestRun parent_;
  std::stringstream json_out_;
  TestFile tmpfile_;
  rpb::OutputArtifact got_;
  TestStep step_;
};

TEST_F(TestStepTest, Begin) {
  // This test also confirms that the readable stream from the child object
  // (TestStep) writes to the same buffer as the parent (TestRun), as expected.
  TestRun test = ResultsTest::GenerateTestRun(
      "Begin", internal::ArtifactWriter(dup(tmpfile_.descriptor), &json_out_));
  test.StartAndRegisterInfos({});
  absl::StatusOr<TestStep> ignored = TestStep::Begin(&test, "BeginTest");

  std::string line;
  std::getline(json_out_, line);  // discard TestRunStart artifact
  std::getline(json_out_, line);
  toProtoOrFail(line, got_);
  std::string want = absl::StrFormat(R"pb(
    test_step_artifact {
      test_step_start { name: "BeginTest" }
      test_step_id: "0"
    }
  )pb");
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

TEST_F(TestStepTest, BeginWithNullParent) {
  std::unique_ptr<TestRun> t = nullptr;
  absl::StatusOr<TestStep> step = TestStep::Begin(t.get(), "invalid");
  ASSERT_THAT(step, StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST_F(TestStepTest, BeginWithParentNotStarted) {
  TestRun test = ResultsTest::GenerateTestRun(
      "Begin", internal::ArtifactWriter(dup(tmpfile_.descriptor), &json_out_));
  absl::StatusOr<TestStep> step = TestStep::Begin(&test, "invalid");
  ASSERT_THAT(step, StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST_F(TestStepTest, AddDiagnosis) {
  internal::ArtifactWriter fakeWriter(tmpfile_.descriptor, &json_out_);
  HwRecord hw =
      ResultsTest::GenerateHwRecord(ParseTextProtoOrDie(kHwRegistered));
  fakeWriter.RegisterHwId(hw.Data().hardware_info_id());
  TestStep step = ResultsTest::GenerateTestStep(&parent_, "AddDiagnosis",
                                                std::move(fakeWriter));
  step.AddDiagnosis(rpb::Diagnosis::PASS, "symptom", "add diag success", {hw});

  toProtoOrFail(json_out_.str(), got_);
  // Compare to what we expect (ignoring timestamp)
  std::string want = absl::StrFormat(R"pb(
                                       test_step_artifact {
                                         diagnosis {
                                           symptom: "symptom"
                                           type: PASS
                                           msg: "add diag success"
                                           hardware_info_id: "%s"
                                         }
                                         test_step_id: "%s"
                                       }
                                       sequence_number: 0
                                     )pb",
                                     hw.Data().hardware_info_id(), step.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

TEST_F(TestStepTest, AddDiagnosis_hw_unregistered) {
  HwRecord hw =
      ResultsTest::GenerateHwRecord(ParseTextProtoOrDie(kHwNotRegistered));
  step_.AddDiagnosis(rpb::Diagnosis::FAIL, "symptom", "add diag fail", {hw});

  std::vector<std::string> wants;
  std::vector<rpb::OutputArtifact> gots_json;
  std::vector<rpb::OutputArtifact> gots_file;
  // Expect Error log artifact first.
  std::string line;
  std::getline(json_out_, line);
  toProtoOrFail(line, got_);
  gots_json.push_back(got_);
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          error { symptom: "%s" }
          test_step_id: "%s"
        }
      )pb",
      kSympUnregHw, step_.Id());
  wants.push_back(want);

  // Then expect diag result with missing hardware_info_id
  std::getline(json_out_, line);
  toProtoOrFail(line, got_);
  gots_json.push_back(got_);
  want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          diagnosis { symptom: "symptom" type: FAIL msg: "add diag fail" }
          test_step_id: "%s"
        }
      )pb",
      step_.Id());
  wants.push_back(want);
  step_.GetWriter().Close();
}

TEST_F(TestStepTest, AddError) {
  internal::ArtifactWriter fakeWriter(tmpfile_.descriptor, &json_out_);
  SwRecord sw =
      ResultsTest::GenerateSwRecord(ParseTextProtoOrDie(kSwRegistered));
  fakeWriter.RegisterSwId(sw.Data().software_info_id());
  TestStep step = ResultsTest::GenerateTestStep(&parent_, "AddError",
                                                std::move(fakeWriter));
  step.AddError("symptom", "add error success", {sw});

  toProtoOrFail(json_out_.str(), got_);
  // Compare to what we expect (ignoring timestamp)
  std::string want = absl::StrFormat(R"pb(
                                       test_step_artifact {
                                         error {
                                           symptom: "symptom"
                                           msg: "add error success"
                                           software_info_id: "%s"
                                         }
                                         test_step_id: "%s"
                                       }
                                       sequence_number: 0
                                     )pb",
                                     sw.Data().software_info_id(), step.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
  ASSERT_EQ(step.Status(), rpb::TestStatus::ERROR);
  //
  // the auto-calc is working
}

TEST_F(TestStepTest, AddError_sw_unregistered) {
  SwRecord sw =
      ResultsTest::GenerateSwRecord(ParseTextProtoOrDie(kSwNotRegistered));
  step_.AddError("symptom", "add error fail", {sw});

  // Expect Error log artifact first.
  std::string line;
  std::getline(json_out_, line);
  toProtoOrFail(line, got_);
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          error { symptom: "%s" }
          test_step_id: "%s"
        }
      )pb",
      kSympUnregSw, step_.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));

  // Then expect error result with missing software_info_id
  std::getline(json_out_, line);
  toProtoOrFail(line, got_);
  want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          error { symptom: "symptom" msg: "add error fail" }
          test_step_id: "%s"
        }
      )pb",
      step_.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
  ASSERT_EQ(step_.Status(), rpb::TestStatus::ERROR);
}

TEST_F(TestStepTest, AddMeasurement) {
  internal::ArtifactWriter fakeWriter(tmpfile_.descriptor, &json_out_);
  HwRecord hw =
      ResultsTest::GenerateHwRecord(ParseTextProtoOrDie(kHwRegistered));
  fakeWriter.RegisterHwId(hw.Data().hardware_info_id());
  TestStep step = ResultsTest::GenerateTestStep(&parent_, "AddMeasurement",
                                                std::move(fakeWriter));
  rpb::MeasurementInfo info;
  info.set_name("measurement info");
  info.set_unit("units");
  rpb::MeasurementElement elem;
  google::protobuf::Value val;
  val.set_number_value(3.14);
  *elem.mutable_valid_values()->add_values() = val;
  *elem.mutable_value() = std::move(val);
  google::protobuf::Timestamp now = Now();
  *elem.mutable_dut_timestamp() = now;

  step.AddMeasurement(info, elem, &hw);

  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          measurement {
            info {
              name: "measurement info"
              unit: "units"
              hardware_info_id: "%s"
            }
            element {
              measurement_series_id: "NOT_APPLICABLE"
              value { number_value: 3.14 }
              valid_values { values { number_value: 3.14 } }
              dut_timestamp { seconds: %d nanos: %d }
            }
          }
          test_step_id: "%s"
        }
      )pb",
      hw.Data().hardware_info_id(), now.seconds(), now.nanos(), step.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

// Expect Error artifact if MeasurementElement is mal-formed (Struct kind not
// allowed)
TEST_F(TestStepTest, AddMeasurement_fail) {
  testonly::MockTestStep mock_step;
  mock_step.DelegateToReal();
  rpb::MeasurementElement elem;
  *elem.mutable_value()->mutable_struct_value() = google::protobuf::Struct();
  EXPECT_CALL(mock_step, AddError(absl::string_view(kSympProceduralErr), _, _))
      .Times(1);
  mock_step.AddMeasurement({}, elem, nullptr);
}

TEST_F(TestStepTest, ValidateMeasElem_invalid_kind) {
  rpb::MeasurementElement elem;
  *elem.mutable_value()->mutable_struct_value() = google::protobuf::Struct();
  absl::Status status = step_.ValidateMeasElem(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(absl::StrContains(status.message(), "value of kind 'Struct'"));
}

TEST_F(TestStepTest, ValidateMeasElem_invalid_range_kind) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_bool_value(
      true);  // valid for values, but not range.
  elem.mutable_range()->mutable_maximum()->set_number_value(0);
  elem.mutable_range()->mutable_minimum()->set_number_value(0);
  absl::Status status = step_.ValidateMeasElem(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(absl::StrContains(status.message(), "value of kind 'bool'"));
}

TEST_F(TestStepTest, ValidateMeasElem_invalid_range_min_kind) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_number_value(0);
  elem.mutable_range()->mutable_maximum()->set_number_value(0);
  elem.mutable_range()->mutable_minimum()->set_bool_value(true);
  absl::Status status = step_.ValidateMeasElem(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(absl::StrContains(status.message(), "value of kind 'bool'"));
}

TEST_F(TestStepTest, ValidateMeasElem_invalid_range_max_kind) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_number_value(0);
  elem.mutable_range()->mutable_maximum()->set_null_value({});
  elem.mutable_range()->mutable_minimum()->set_number_value(0);
  absl::Status status = step_.ValidateMeasElem(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(absl::StrContains(status.message(), "value of kind 'NullValue'"));
}

TEST_F(TestStepTest, ValidateMeasElem_values_mismatch) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_bool_value(true);
  elem.mutable_valid_values()->mutable_values()->Add(google::protobuf::Value{});
  absl::Status status = step_.ValidateMeasElem(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(
      absl::StrContains(status.message(), "value of kind 'kind not set'"));
}

TEST_F(TestStepTest, ValidateMeasElem_range_min_mismatch) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_string_value("");
  elem.mutable_range()->mutable_maximum()->set_string_value("");
  elem.mutable_range()->mutable_minimum()->set_number_value(0);
  absl::Status status = step_.ValidateMeasElem(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(
      absl::StrContains(status.message(), "'double' does not equal 'string'"));
}

TEST_F(TestStepTest, ValidateMeasElem_range_max_mismatch) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_string_value("");
  elem.mutable_range()->mutable_maximum()->set_number_value(0);
  elem.mutable_range()->mutable_minimum()->set_string_value("");
  absl::Status status = step_.ValidateMeasElem(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(
      absl::StrContains(status.message(), "'double' does not equal 'string'"));
}

TEST_F(TestStepTest, ValidateMeasElem_range_max_empty) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_string_value("");
  elem.mutable_range()->mutable_minimum()->set_string_value("");
  EXPECT_OK(step_.ValidateMeasElem(elem));
}

TEST_F(TestStepTest, AddMeasurementWithNullptr) {
  internal::ArtifactWriter fakeWriter(tmpfile_.descriptor, &json_out_);
  TestStep step = ResultsTest::GenerateTestStep(&parent_, "AddMeasurement",
                                                std::move(fakeWriter));
  rpb::MeasurementInfo info;
  info.set_name("measurement info");
  info.set_unit("units");
  rpb::MeasurementElement elem;
  google::protobuf::Value val;
  val.set_number_value(3.14);
  *elem.mutable_valid_values()->add_values() = val;
  *elem.mutable_value() = std::move(val);
  google::protobuf::Timestamp now = Now();
  *elem.mutable_dut_timestamp() = now;

  step.AddMeasurement(info, elem, nullptr);

  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          measurement {
            info { name: "measurement info" unit: "units" }
            element {
              measurement_series_id: "NOT_APPLICABLE"
              value { number_value: 3.14 }
              valid_values { values { number_value: 3.14 } }
              dut_timestamp { seconds: %d nanos: %d }
            }
          }
          test_step_id: "%s"
        }
      )pb",
      now.seconds(), now.nanos(), step.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

TEST_F(TestStepTest, AddMeasurement_hw_unregistered) {
  internal::ArtifactWriter fakeWriter(tmpfile_.descriptor, &json_out_);
  HwRecord hw =
      ResultsTest::GenerateHwRecord(ParseTextProtoOrDie(kHwRegistered));
  TestStep step = ResultsTest::GenerateTestStep(&parent_, "AddMeasurement",
                                                std::move(fakeWriter));
  rpb::MeasurementInfo info;
  info.set_name("measurement info");
  info.set_unit("units");
  rpb::MeasurementElement elem;
  google::protobuf::Value val;
  val.set_number_value(3.14);
  *elem.mutable_valid_values()->add_values() = val;
  *elem.mutable_value() = std::move(val);
  google::protobuf::Timestamp now = Now();
  *elem.mutable_dut_timestamp() = now;

  step.AddMeasurement(info, elem, &hw);

  // Expect Error Log artifact
  std::string line;
  std::getline(json_out_, line);
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          error { symptom: "%s" }
          test_step_id: "%s"
        }
      )pb",
      kSympUnregHw, step.Id());
  toProtoOrFail(line, got_);
  EXPECT_THAT(got_, Partially(EqualsProto(want)));

  std::getline(json_out_, line);
  toProtoOrFail(line, got_);
  want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          measurement {
            info { name: "measurement info" unit: "units" }
            element {
              measurement_series_id: "NOT_APPLICABLE"
              value { number_value: 3.14 }
              valid_values { values { number_value: 3.14 } }
              dut_timestamp { seconds: %d nanos: %d }
            }
          }
          test_step_id: "%s"
        }
      )pb",
      now.seconds(), now.nanos(), step.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

TEST_F(TestStepTest, AddFile) {
  rpb::File file;
  file.set_upload_as_name("upload name");
  file.set_output_path("out path");
  file.set_description("description");
  file.set_content_type("content type");
  file.set_is_snapshot(true);
  step_.AddFile(file);

  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          file {
            upload_as_name: "upload name"
            output_path: "out path"
            description: "description"
            content_type: "content type"
            is_snapshot: true
          }
          test_step_id: "%s"
        }
      )pb",
      step_.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

TEST_F(TestStepTest, AddArtifactExtension) {
  google::protobuf::Empty extension;
  step_.AddArtifactExtension("test extension", extension);

  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          extension {
            name: "test extension"
            extension {
              type_url: "type.googleapis.com/google.protobuf.Empty"
              value: "%s"
            }
          }
          test_step_id: "%s"
        }
        sequence_number: 0
      )pb",
      extension.SerializeAsString(), step_.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

TEST_F(TestStepTest, End) {
  ASSERT_FALSE(step_.Ended());
  ASSERT_EQ(step_.Status(), rpb::TestStatus::UNKNOWN);
  step_.End();

  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          test_step_end { name: "TestStepTest" status: COMPLETE }
          test_step_id: "%s"
        }
        sequence_number: 0
      )pb",
      step_.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
  ASSERT_EQ(step_.Status(), rpb::TestStatus::COMPLETE);
  ASSERT_TRUE(step_.Ended());
}

TEST_F(TestStepTest, Skip) {
  ASSERT_EQ(step_.Status(), rpb::TestStatus::UNKNOWN);
  step_.Skip();
  ASSERT_EQ(step_.Status(), rpb::TestStatus::SKIPPED);
  ASSERT_TRUE(step_.Ended());
}

TEST_F(TestStepTest, LogDebug) {
  step_.LogDebug("my house has termites, please debug it");
  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          log { severity: DEBUG text: "my house has termites, please debug it" }
          test_step_id: "%s"
        }
      )pb",
      step_.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

TEST_F(TestStepTest, LogInfo) {
  step_.LogInfo("Here, have an info");
  toProtoOrFail(json_out_.str(), got_);
  std::string want =
      absl::StrFormat(R"pb(
                        test_step_artifact {
                          log { severity: INFO text: "Here, have an info" }
                          test_step_id: "%s"
                        }
                      )pb",
                      step_.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

TEST_F(TestStepTest, LogWarn) {
  step_.LogWarn("this test is brand new, never warn");
  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          log { severity: WARNING text: "this test is brand new, never warn" }
          test_step_id: "%s"
        }
      )pb",
      step_.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

TEST_F(TestStepTest, LogError) {
  step_.LogError("to err is human");
  toProtoOrFail(json_out_.str(), got_);
  std::string want =
      absl::StrFormat(R"pb(
                        test_step_artifact {
                          log { severity: ERROR text: "to err is human" }
                          test_step_id: "%s"
                        }
                      )pb",
                      step_.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

TEST_F(TestStepTest, LogFatal) {
  step_.LogFatal("What's my destiny? Fatal determine that");
  toProtoOrFail(json_out_.str(), got_);
  std::string want =
      absl::StrFormat(R"pb(
                        test_step_artifact {
                          log {
                            severity: FATAL
                            text: "What's my destiny? Fatal determine that"
                          }
                          test_step_id: "%s"
                        }
                      )pb",
                      step_.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
  //
}

TEST(DutInfoTest, Add) {
  DutInfo di("Test host");
  di.AddPlatformInfo("plugin:something");
  di.AddPlatformInfo("kernel_version:1.0.0");
  HwRecord hw = di.AddHardware(ParseTextProtoOrDie(kHwRegistered));
  SwRecord sw = di.AddSoftware(ParseTextProtoOrDie(kSwRegistered));
  rpb::DutInfo got = di.ToProto();
  std::string want = absl::StrFormat(
      R"pb(
        hostname: "Test host"
        platform_info { info: "plugin:something" info: "kernel_version:1.0.0" }
        hardware_components { %s }
        software_infos { %s }
      )pb",
      absl::StrFormat(kHwRegistered, hw.Data().hardware_info_id()),
      absl::StrFormat(kSwRegistered, sw.Data().software_info_id()));
  EXPECT_THAT(got, EqualsProto(want));
}

// Ensures HwRecords receive globally unique IDs even with many DutInfos in
// different threads.
TEST(DutInfoTest, ID_Uniqueness) {
  constexpr int kDutInfoCount = 20;
  constexpr int kInfosPer = 10000;
  constexpr int kWantCount = kDutInfoCount * kInfosPer;
  const rpb::HardwareInfo kInfo;
  absl::Mutex mu_;
  absl::flat_hash_set<std::string> ids;
  ids.reserve(kWantCount);
  std::vector<std::future<void>> threads(kDutInfoCount);
  for (int i = 0; i < kDutInfoCount; i++) {
    threads[i] = std::async(std::launch::async, [&, i] {
      for (int j = 0; j < kInfosPer; j++) {
        DutInfo di(absl::StrCat("host", i));
        HwRecord record = di.AddHardware(kInfo);
        mu_.Lock();
        ids.insert(record.Data().hardware_info_id());
        mu_.Unlock();
      }
    });
  }
  for (std::future<void>& thread : threads) {
    thread.wait();
  }
  EXPECT_EQ(kWantCount, ids.size());
}

// Test fixture for MeasurementSeries
class MeasurementSeriesTest : public ::testing::Test {
 protected:
  MeasurementSeriesTest()
      : series_(ResultsTest::GenerateMeasurementSeries(
            internal::ArtifactWriter(dup(tmpfile_.descriptor), &json_out_))) {}
  void SetUp() { json_out_.clear(); }
  ~MeasurementSeriesTest() override = default;

  std::stringstream json_out_;
  TestFile tmpfile_;
  rpb::OutputArtifact got_;
  MeasurementSeries series_;
};

TEST_F(MeasurementSeriesTest, Begin) {
  internal::ArtifactWriter fakeWriter(tmpfile_.descriptor, &json_out_);
  HwRecord hw =
      ResultsTest::GenerateHwRecord(ParseTextProtoOrDie(kHwRegistered));
  std::string hw_id = hw.Data().hardware_info_id();
  fakeWriter.RegisterHwId(hw_id);
  TestStep step = ResultsTest::GenerateTestStep(
      nullptr, "MeasurementSeries::Begin", std::move(fakeWriter));
  rpb::MeasurementInfo minfo =
      ParseTextProtoOrDie(absl::StrFormat(kMeasurementInfo, hw_id));
  absl::StatusOr<MeasurementSeries> series =
      MeasurementSeries::Begin(&step, hw, minfo);
  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          measurement_series_start { measurement_series_id: "%s" info { %s } }
          test_step_id: "%s"
        }
      )pb",
      series->Id(), absl::StrFormat(kMeasurementInfo, hw_id), step.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

// Asserts that a MeasurementSeries cannot be started with a step that already
// ended.
TEST_F(MeasurementSeriesTest, Begin_fail_after_step_end) {
  TestStep step = ResultsTest::GenerateTestStep(
      nullptr, "", internal::ArtifactWriter(-1, nullptr));
  step.End();
  HwRecord hw = ResultsTest::GenerateHwRecord(rpb::HardwareInfo());
  absl::StatusOr<MeasurementSeries> ser =
      MeasurementSeries::Begin(&step, hw, rpb::MeasurementInfo());
  ASSERT_THAT(ser, StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST_F(MeasurementSeriesTest, Begin_hw_unregistered) {
  internal::ArtifactWriter fakeWriter(tmpfile_.descriptor, &json_out_);
  TestStep step = ResultsTest::GenerateTestStep(
      nullptr, "MeasurementSeries::Begin", std::move(fakeWriter));
  HwRecord hw =
      ResultsTest::GenerateHwRecord(ParseTextProtoOrDie(kHwNotRegistered));
  rpb::MeasurementInfo minfo = ParseTextProtoOrDie(kMeasurementInfo);
  absl::StatusOr<MeasurementSeries> series =
      MeasurementSeries::Begin(&step, hw, minfo);
  std::string line;
  std::getline(json_out_, line);
  toProtoOrFail(line, got_);
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          error { symptom: "%s" }
          test_step_id: "%s"
        }
      )pb",
      kSympUnregHw, step.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

TEST_F(MeasurementSeriesTest, Begin_null_parent) {
  std::unique_ptr<TestStep> s = nullptr;
  rpb::MeasurementInfo info;
  absl::StatusOr<MeasurementSeries> series = MeasurementSeries::Begin(
      s.get(), ResultsTest::GenerateHwRecord(rpb::HardwareInfo()), info);
  ASSERT_THAT(series, StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST_F(MeasurementSeriesTest, AddElement_without_limit) {
  google::protobuf::Value val;
  val.set_number_value(3.14);
  series_.AddElement(val);

  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          measurement_element {
            measurement_series_id: "%s"
            value { number_value: 3.14 }
          }
          test_step_id: "%s"
        }
      )pb",
      series_.Id(), kStepIdDefault);
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

TEST_F(MeasurementSeriesTest, AddElement_with_range) {
  google::protobuf::Value val, min, max;
  rpb::MeasurementElement::Range range;
  min.set_number_value(3.13);
  val.set_number_value(3.14);
  max.set_number_value(3.15);
  *range.mutable_minimum() = std::move(min);
  *range.mutable_maximum() = std::move(max);
  series_.AddElementWithRange(val, range);

  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          measurement_element {
            measurement_series_id: "%s"
            value { number_value: 3.14 }
            range {
              maximum { number_value: 3.15 }
              minimum { number_value: 3.13 }
            }
          }
          test_step_id: "%s"
        }
      )pb",
      series_.Id(), kStepIdDefault);
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

// ERROR if Range used as limit for ListValue
TEST_F(MeasurementSeriesTest, value_kind_list_with_range_limit) {
  testonly::MockTestStep step;
  testonly::MockMeasurementSeries series(&step);
  series.DelegateToReal();
  google::protobuf::Value val, list;
  val.set_number_value(0);
  *list.mutable_list_value()->add_values() = std::move(val);
  EXPECT_CALL(step, AddError(absl::string_view(kSympProceduralErr), _, _))
      .Times(1);
  series.AddElementWithRange(list, {});
}

// ERROR if Value kinds don't match
TEST_F(MeasurementSeriesTest, value_kind_mismatch) {
  testonly::MockTestStep step;
  testonly::MockMeasurementSeries series(&step);
  series.DelegateToReal();
  google::protobuf::Value number_val, string_val;
  number_val.set_number_value(0);
  string_val.set_string_value("zero");
  EXPECT_CALL(step, AddError(absl::string_view(kSympProceduralErr), _, _))
      .Times(1);
  series.AddElementWithValues(number_val, {string_val});
}

// ERROR if Value has no type
TEST_F(MeasurementSeriesTest, value_kind_no_type) {
  testonly::MockTestStep step;
  testonly::MockMeasurementSeries series(&step);
  series.DelegateToReal();
  google::protobuf::Value no_type;
  EXPECT_CALL(step, AddError(absl::string_view(kSympProceduralErr), _, _))
      .Times(1);
  series.AddElementWithValues(no_type, {no_type});
}

// ERROR if Value has no type
TEST_F(MeasurementSeriesTest, value_kind_reject_struct) {
  testonly::MockTestStep step;
  testonly::MockMeasurementSeries series(&step);
  series.DelegateToReal();
  google::protobuf::Value struct_type;
  *struct_type.mutable_struct_value() = google::protobuf::Struct();
  EXPECT_CALL(step, AddError(absl::string_view(kSympProceduralErr), _, _))
      .Times(1);
  series.AddElementWithValues(struct_type, {struct_type});
}

TEST_F(MeasurementSeriesTest, AddElement_with_valid_values) {
  google::protobuf::Value val;
  val.set_number_value(3.14);
  series_.AddElementWithValues(val, {val});

  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          measurement_element {
            measurement_series_id: "%s"
            value { number_value: 3.14 }
            valid_values { values { number_value: 3.14 } }
          }
          test_step_id: "%s"
        }
      )pb",
      series_.Id(), kStepIdDefault);
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

// Ensures MeasurementElements cannot be added to a series after it has ended.
TEST_F(MeasurementSeriesTest, AddElementWithValues_fail_after_ended) {
  series_.End();
  google::protobuf::Value val;
  rpb::MeasurementElement::Range range;
  range.mutable_maximum()->set_number_value(0);
  range.mutable_minimum()->set_number_value(0);
  val.set_number_value(0);
  series_.AddElementWithValues(val, {val});
  series_.AddElementWithRange(val, range);
  // expect only 1 artifact from End(). The following will not parse if
  // extra artifacts are added from AddElement*().
  toProtoOrFail(json_out_.str(), got_);
}

TEST_F(MeasurementSeriesTest, End) {
  ASSERT_FALSE(series_.Ended());
  series_.End();
  ASSERT_TRUE(series_.Ended());
  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(R"pb(
                                       test_step_artifact {
                                         measurement_series_end {
                                           measurement_series_id: "%s"
                                           total_measurement_count: 0
                                         }
                                         test_step_id: "%s"
                                       }
                                     )pb",
                                     series_.Id(), kStepIdDefault);
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

}  // namespace internal
}  // namespace results
}  // namespace meltan
