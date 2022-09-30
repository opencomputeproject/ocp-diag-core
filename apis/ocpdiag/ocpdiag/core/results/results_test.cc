// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/results.h"

#include <unistd.h>

#include <filesystem>  //
#include <fstream>
#include <future>  //
#include <memory>
#include <sstream>
#include <string>
#include <system_error>  //
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
#include "ocpdiag/core/compat/status_converters.h"
#include "ocpdiag/core/results/internal/logging.h"
#include "ocpdiag/core/results/internal/mock_file_handler.h"
#include "ocpdiag/core/results/internal/test_utils.h"
#include "ocpdiag/core/results/results.pb.h"
#include "ocpdiag/core/testing/mock_results.h"
#include "ocpdiag/core/testing/parse_text_proto.h"
#include "ocpdiag/core/testing/proto_matchers.h"
#include "ocpdiag/core/testing/status_matchers.h"

namespace ocpdiag {
namespace results {
namespace internal {

using internal::ArtifactWriter;
using internal::Now;
using internal::TestFile;
using ::ocpdiag::testing::EqualsProto;
using ::ocpdiag::testing::ParseTextProtoOrDie;
using ::ocpdiag::testing::Partially;
using ::ocpdiag::testing::StatusIs;
using ::testing::_;
using ::testing::Return;

namespace rpb = ::ocpdiag::results_pb;

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
  static TestRun GenerateTestRun(std::string name,
                                 std::shared_ptr<ArtifactWriter> w) {
    return TestRun(name, w);
  }

  static HwRecord GenerateHwRecord(rpb::HardwareInfo info) {
    DutInfo di("hostname");
    return di.AddHardware(info);
  }

  static SwRecord GenerateSwRecord(rpb::SoftwareInfo info) {
    DutInfo di("hostname");
    return di.AddSoftware(info);
  }

  static TestStep GenerateTestStep(
      TestRun* parent, std::string name,
      std::shared_ptr<internal::ArtifactWriter> writer,
      std::string customId = kStepIdDefault,
      std::unique_ptr<internal::MockFileHandler> file_handler =
          std::make_unique<internal::MockFileHandler>()) {
    return TestStep(parent, customId, std::move(name), std::move(writer),
                    std::move(file_handler));
  }

  static MeasurementSeries GenerateMeasurementSeries(
      std::shared_ptr<internal::ArtifactWriter> writer) {
    rpb::HardwareInfo hwinfo = ParseTextProtoOrDie(kHwRegistered);
    HwRecord hw = GenerateHwRecord(hwinfo);
    rpb::MeasurementInfo minfo = ParseTextProtoOrDie(kMeasurementInfo);
    return MeasurementSeries(nullptr, kStepIdDefault, "series_id",
                             std::move(writer), minfo);
  }

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
      : artifact_writer_(std::make_shared<internal::ArtifactWriter>(
            dup(tmpfile_.descriptor), &json_out_)),
        test_(ResultsTest::GenerateTestRun("TestRunTest", artifact_writer_)) {}
  void SetUp() { json_out_.clear(); }
  ~TestRunTest() override = default;

  std::stringstream json_out_;
  TestFile tmpfile_;
  std::shared_ptr<internal::ArtifactWriter> artifact_writer_;

  rpb::OutputArtifact got_;
  TestRun test_;
  ResultApi api_;
};

TEST_F(TestRunTest, InitOnlyOnce) {
  absl::StatusOr<std::unique_ptr<TestRun>> test =
      api_.InitializeTestRun("orig");
  ASSERT_OK(test);
  absl::StatusOr<std::unique_ptr<TestRun>> dupe =
      api_.InitializeTestRun("dupe");
  ASSERT_THAT(dupe, StatusIs(absl::StatusCode::kAlreadyExists));
}

TEST_F(TestRunTest, StartAndRegisterInfos) {
  ASSERT_FALSE(test_.Started());
  DutInfo di("host");
  DutInfo di2("host2");
  HwRecord hw = di.AddHardware(ParseTextProtoOrDie(kHwRegistered));
  SwRecord sw = di.AddSoftware(ParseTextProtoOrDie(kSwRegistered));
  di.AddPlatformInfo("plugin:something");

  std::vector<ocpdiag::results::DutInfo> dutinfos;
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
  test_.GetWriter()->Close();
}

// Even if TestRun::End() called multiple times, expect only 1 artifact
TEST_F(TestRunTest, EndTwice) {
  test_.End();
  test_.End();
  test_.GetWriter()->Close();
}

// Expect default NOT_APPLICABLE:UNKNOWN
TEST_F(TestRunTest, ResultCalcDefaults) {
  EXPECT_EQ(rpb::TestStatus_Name(test_.Status()),
            rpb::TestStatus_Name(rpb::TestStatus::UNKNOWN));
  EXPECT_EQ(rpb::TestResult_Name(test_.Result()),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
}

// Expect NOT_APPLICABLE:SKIPPED if skipped and no errors emitted
TEST_F(TestRunTest, ResultCalcSkip) {
  rpb::TestResult result = test_.Skip();
  EXPECT_EQ(rpb::TestStatus_Name(test_.Status()),
            rpb::TestStatus_Name(rpb::TestStatus::SKIPPED));
  EXPECT_EQ(rpb::TestResult_Name(test_.Result()),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
  EXPECT_EQ(rpb::TestResult_Name(result),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
}

// Expect NOT_APPLICABLE:ERROR even if skip called after error
TEST_F(TestRunTest, ResultCalcErrorThenSkip) {
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
TEST_F(TestRunTest, ResultCalcPass) {
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
TEST_F(TestRunTest, ResultCalcAddFailDiag) {
  test_.StartAndRegisterInfos({});
  TestStep step = ResultsTest::GenerateTestStep(
      &test_, "", std::make_shared<internal::ArtifactWriter>(-1, nullptr));
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
TEST_F(TestRunTest, ResultCalcTestRunAddError) {
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
TEST_F(TestRunTest, ResultCalcTestStepAddError) {
  test_.StartAndRegisterInfos({});
  TestStep step = ResultsTest::GenerateTestStep(
      &test_, "", std::make_shared<internal::ArtifactWriter>(-1, nullptr));
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
      : artifact_writer_(std::make_shared<internal::ArtifactWriter>(
            dup(tmpfile_.descriptor), &json_out_)),
        parent_(ResultsTest::GenerateTestRun("TestRun", artifact_writer_)),
        step_(ResultsTest::GenerateTestStep(&parent_, "TestStepTest",
                                            artifact_writer_)) {}
  void SetUp() { json_out_.clear(); }
  ~TestStepTest() override = default;

  testonly::FakeResultApi fake_api_;
  std::stringstream json_out_;
  TestFile tmpfile_;
  std::shared_ptr<internal::ArtifactWriter> artifact_writer_;
  TestRun parent_;
  rpb::OutputArtifact got_;
  TestStep step_;
};

TEST_F(TestStepTest, Begin) {
  // This test also confirms that the readable stream from the child object
  // (TestStep) writes to the same buffer as the parent (TestRun), as expected.
  auto artifact_writer_ =
      std::make_shared<ArtifactWriter>(dup(tmpfile_.descriptor), &json_out_);
  TestRun test = ResultsTest::GenerateTestRun("Begin", artifact_writer_);
  test.StartAndRegisterInfos({});
  fake_api_.BeginTestStep(&test, "BeginTest").IgnoreError();

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
  absl::StatusOr<std::unique_ptr<TestStep>> step =
      fake_api_.BeginTestStep(t.get(), "invalid");
  ASSERT_THAT(step, StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST_F(TestStepTest, BeginWithParentNotStarted) {
  auto artifact_writer_ =
      std::make_shared<ArtifactWriter>(dup(tmpfile_.descriptor), &json_out_);
  TestRun test = ResultsTest::GenerateTestRun("Begin", artifact_writer_);
  absl::StatusOr<std::unique_ptr<TestStep>> step =
      fake_api_.BeginTestStep(&test, "invalid");
  ASSERT_THAT(step, StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST_F(TestStepTest, AddDiagnosis) {
  auto fakeWriter =
      std::make_shared<ArtifactWriter>(tmpfile_.descriptor, &json_out_);
  HwRecord hw =
      ResultsTest::GenerateHwRecord(ParseTextProtoOrDie(kHwRegistered));
  fakeWriter->RegisterHwId(hw.Data().hardware_info_id());
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

TEST_F(TestStepTest, AddDiagnosisHwUnregistered) {
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
  step_.GetWriter()->Close();
}

TEST_F(TestStepTest, AddError) {
  auto fakeWriter =
      std::make_shared<ArtifactWriter>(tmpfile_.descriptor, &json_out_);
  SwRecord sw =
      ResultsTest::GenerateSwRecord(ParseTextProtoOrDie(kSwRegistered));
  fakeWriter->RegisterSwId(sw.Data().software_info_id());
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

TEST_F(TestStepTest, AddErrorSwUnregistered) {
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
  auto fakeWriter =
      std::make_shared<ArtifactWriter>(tmpfile_.descriptor, &json_out_);
  HwRecord hw =
      ResultsTest::GenerateHwRecord(ParseTextProtoOrDie(kHwRegistered));
  fakeWriter->RegisterHwId(hw.Data().hardware_info_id());
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
              measurement_series_id: ""
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
TEST_F(TestStepTest, AddMeasurementFail) {
  testonly::MockTestStep mock_step;
  mock_step.DelegateToReal();
  rpb::MeasurementElement elem;
  *elem.mutable_value()->mutable_struct_value() = google::protobuf::Struct();
  EXPECT_CALL(mock_step, AddError(absl::string_view(kSympProceduralErr), _, _))
      .Times(1);
  mock_step.AddMeasurement({}, elem, nullptr);
}

TEST_F(TestStepTest, ValidateMeasElemInvalidKind) {
  rpb::MeasurementElement elem;
  *elem.mutable_value()->mutable_struct_value() = google::protobuf::Struct();
  absl::Status status = step_.ValidateMeasElem(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(absl::StrContains(status.message(), "value of kind 'Struct'"));
}

TEST_F(TestStepTest, ValidateMeasElemInvalidRangeKind) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_bool_value(
      true);  // valid for values, but not range.
  elem.mutable_range()->mutable_maximum()->set_number_value(0);
  elem.mutable_range()->mutable_minimum()->set_number_value(0);
  absl::Status status = step_.ValidateMeasElem(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(absl::StrContains(status.message(), "value of kind 'bool'"));
}

TEST_F(TestStepTest, ValidateMeasElemInvalidRangeMinKind) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_number_value(0);
  elem.mutable_range()->mutable_maximum()->set_number_value(0);
  elem.mutable_range()->mutable_minimum()->set_bool_value(true);
  absl::Status status = step_.ValidateMeasElem(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(absl::StrContains(status.message(), "value of kind 'bool'"));
}

TEST_F(TestStepTest, ValidateMeasElemInvalidRangeMaxKind) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_number_value(0);
  elem.mutable_range()->mutable_maximum()->set_null_value({});
  elem.mutable_range()->mutable_minimum()->set_number_value(0);
  absl::Status status = step_.ValidateMeasElem(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(absl::StrContains(status.message(), "value of kind 'NullValue'"));
}

TEST_F(TestStepTest, ValidateMeasElemValuesMismatch) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_bool_value(true);
  elem.mutable_valid_values()->mutable_values()->Add(google::protobuf::Value{});
  absl::Status status = step_.ValidateMeasElem(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(
      absl::StrContains(status.message(), "value of kind 'kind not set'"));
}

TEST_F(TestStepTest, ValidateMeasElemRangeMinMismatch) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_string_value("");
  elem.mutable_range()->mutable_maximum()->set_string_value("");
  elem.mutable_range()->mutable_minimum()->set_number_value(0);
  absl::Status status = step_.ValidateMeasElem(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(
      absl::StrContains(status.message(), "'double' does not equal 'string'"));
}

TEST_F(TestStepTest, ValidateMeasElemRangeMaxMismatch) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_string_value("");
  elem.mutable_range()->mutable_maximum()->set_number_value(0);
  elem.mutable_range()->mutable_minimum()->set_string_value("");
  absl::Status status = step_.ValidateMeasElem(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(
      absl::StrContains(status.message(), "'double' does not equal 'string'"));
}

TEST_F(TestStepTest, ValidateMeasElemRangeMaxEmpty) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_string_value("");
  elem.mutable_range()->mutable_minimum()->set_string_value("");
  EXPECT_OK(step_.ValidateMeasElem(elem));
}

TEST_F(TestStepTest, AddMeasurementWithNullptr) {
  auto fakeWriter =
      std::make_shared<ArtifactWriter>(tmpfile_.descriptor, &json_out_);
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
              measurement_series_id: ""
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

TEST_F(TestStepTest, AddMeasurementHwUnregistered) {
  auto fakeWriter =
      std::make_shared<ArtifactWriter>(tmpfile_.descriptor, &json_out_);
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
              measurement_series_id: ""
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

// Test AddFile on local CWD file, no file copy expected.
TEST_F(TestStepTest, AddFile) {
  auto fh = std::make_unique<internal::MockFileHandler>();
  // Make sure copy methods are not called.
  EXPECT_CALL(*fh, CopyLocalFile(_, _)).Times(0);
  EXPECT_CALL(*fh, CopyRemoteFile(_)).Times(0);
  auto step =
      ResultsTest::GenerateTestStep(nullptr, "step",
                                    std::make_shared<internal::ArtifactWriter>(
                                        dup(tmpfile_.descriptor), &json_out_),
                                    kStepIdDefault, std::move(fh));

  std::error_code err;
  std::string cwd = std::filesystem::current_path(err);
  ASSERT_FALSE(err);

  std::string output_path = absl::StrCat(cwd, "cwdfile.LOG");
  rpb::File file;
  file.set_upload_as_name("upload name");
  file.set_output_path(output_path);
  file.set_description("description");
  file.set_content_type("content type");
  step.AddFile(file);

  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          file {
            upload_as_name: "upload name"
            output_path: "%s"
            description: "description"
            content_type: "content type"
          }
          test_step_id: "%s"
        }
      )pb",
      output_path, step.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

// expect local file copy
TEST_F(TestStepTest, AddFileLocalCopy) {
  auto fh = std::make_unique<internal::MockFileHandler>();
  EXPECT_CALL(*fh, CopyLocalFile(_, _)).WillOnce(Return(absl::OkStatus()));
  auto step =
      ResultsTest::GenerateTestStep(nullptr, "step",
                                    std::make_shared<internal::ArtifactWriter>(
                                        dup(tmpfile_.descriptor), &json_out_),
                                    kStepIdDefault, std::move(fh));
  rpb::File file;
  // Set a local path not in test dir, so expects file copy.
  file.set_output_path("/tmp/data/file");
  step.AddFile(file);

  // Skip proto output comparison, this is tested in FileHandler
}

TEST_F(TestStepTest, AddFileFail) {
  auto fh = std::make_unique<internal::MockFileHandler>();
  EXPECT_CALL(*fh, CopyLocalFile(_, _))
      .WillOnce(Return(absl::UnknownError("")));
  auto step =
      ResultsTest::GenerateTestStep(nullptr, "step",
                                    std::make_shared<internal::ArtifactWriter>(
                                        dup(tmpfile_.descriptor), &json_out_),
                                    kStepIdDefault, std::move(fh));
  rpb::File file;
  file.set_output_path("/tmp/data/file");
  step.AddFile(file);

  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          error { symptom: "ocpdiag-internal-error" }
          test_step_id: "%s"
        }
      )pb",
      step.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

TEST_F(TestStepTest, AddFileRemote) {
  auto fh = std::make_unique<internal::MockFileHandler>();
  EXPECT_CALL(*fh, CopyRemoteFile(_)).WillOnce(Return(absl::OkStatus()));
  auto step =
      ResultsTest::GenerateTestStep(nullptr, "step",
                                    std::make_shared<internal::ArtifactWriter>(
                                        dup(tmpfile_.descriptor), &json_out_),
                                    kStepIdDefault, std::move(fh));
  rpb::File file;
  file.set_upload_as_name("upload name");
  // mimic absolute path on remote node.
  file.set_output_path("/tmp/data/file");
  file.set_description("description");
  file.set_content_type("content type");
  file.set_node_address("node_address");
  step.AddFile(file);
}

TEST_F(TestStepTest, AddFileRemoteFail) {
  auto fh = std::make_unique<internal::MockFileHandler>();
  EXPECT_CALL(*fh, CopyRemoteFile(_)).WillOnce(Return(absl::UnknownError("")));
  auto step =
      ResultsTest::GenerateTestStep(nullptr, "step",
                                    std::make_shared<internal::ArtifactWriter>(
                                        dup(tmpfile_.descriptor), &json_out_),
                                    kStepIdDefault, std::move(fh));
  rpb::File file;
  file.set_upload_as_name("upload name");
  // mimic absolute path on remote node.
  file.set_output_path("/tmp/data/file");
  file.set_description("description");
  file.set_content_type("content type");
  file.set_node_address("node_address");
  step.AddFile(file);

  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          error { symptom: "ocpdiag-internal-error" }
          test_step_id: "%s"
        }
      )pb",
      step.Id());
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
            std::make_shared<internal::ArtifactWriter>(dup(tmpfile_.descriptor),
                                                       &json_out_))) {}
  void SetUp() { json_out_.clear(); }
  ~MeasurementSeriesTest() override = default;

  testonly::FakeResultApi fake_api_;
  std::stringstream json_out_;
  TestFile tmpfile_;
  rpb::OutputArtifact got_;
  MeasurementSeries series_;
};

TEST_F(MeasurementSeriesTest, Begin) {
  auto fakeWriter =
      std::make_shared<ArtifactWriter>(tmpfile_.descriptor, &json_out_);
  HwRecord hw =
      ResultsTest::GenerateHwRecord(ParseTextProtoOrDie(kHwRegistered));
  std::string hw_id = hw.Data().hardware_info_id();
  fakeWriter->RegisterHwId(hw_id);
  TestStep step = ResultsTest::GenerateTestStep(
      nullptr, "fake_api_.BeginMeasurementSeries", std::move(fakeWriter));
  rpb::MeasurementInfo minfo =
      ParseTextProtoOrDie(absl::StrFormat(kMeasurementInfo, hw_id));
  absl::StatusOr<std::unique_ptr<MeasurementSeries>> series =
      fake_api_.BeginMeasurementSeries(&step, hw, minfo);
  ASSERT_OK(series);
  toProtoOrFail(json_out_.str(), got_);
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          measurement_series_start { measurement_series_id: "%s" info { %s } }
          test_step_id: "%s"
        }
      )pb",
      (*series)->Id(), absl::StrFormat(kMeasurementInfo, hw_id), step.Id());
  EXPECT_THAT(got_, Partially(EqualsProto(want)));
}

// Asserts that a MeasurementSeries cannot be started with a step that already
// ended.
TEST_F(MeasurementSeriesTest, BeginFailAfterStepEnd) {
  TestStep step = ResultsTest::GenerateTestStep(
      nullptr, "", std::make_shared<internal::ArtifactWriter>(-1, nullptr));
  step.End();
  HwRecord hw = ResultsTest::GenerateHwRecord(rpb::HardwareInfo());
  ASSERT_THAT(
      fake_api_.BeginMeasurementSeries(&step, hw, rpb::MeasurementInfo()),
      StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST_F(MeasurementSeriesTest, BeginHwUnregistered) {
  auto fakeWriter =
      std::make_shared<ArtifactWriter>(tmpfile_.descriptor, &json_out_);
  TestStep step = ResultsTest::GenerateTestStep(
      nullptr, "fake_api_.BeginMeasurementSeries", std::move(fakeWriter));
  HwRecord hw =
      ResultsTest::GenerateHwRecord(ParseTextProtoOrDie(kHwNotRegistered));
  rpb::MeasurementInfo minfo = ParseTextProtoOrDie(kMeasurementInfo);
  absl::StatusOr<std::unique_ptr<MeasurementSeries>> series =
      fake_api_.BeginMeasurementSeries(&step, hw, minfo);
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

TEST_F(MeasurementSeriesTest, BeginNullParent) {
  std::unique_ptr<TestStep> s = nullptr;
  rpb::MeasurementInfo info;
  ASSERT_THAT(
      fake_api_.BeginMeasurementSeries(
          s.get(), ResultsTest::GenerateHwRecord(rpb::HardwareInfo()), info),
      StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST_F(MeasurementSeriesTest, AddElementWithoutLimit) {
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

TEST_F(MeasurementSeriesTest, AddElementWithRange) {
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
TEST_F(MeasurementSeriesTest, ValueKindListWithRangeLimit) {
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
TEST_F(MeasurementSeriesTest, ValueKindMismatch) {
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
TEST_F(MeasurementSeriesTest, ValueKindNoType) {
  testonly::MockTestStep step;
  testonly::MockMeasurementSeries series(&step);
  series.DelegateToReal();
  google::protobuf::Value no_type;
  EXPECT_CALL(step, AddError(absl::string_view(kSympProceduralErr), _, _))
      .Times(1);
  series.AddElementWithValues(no_type, {no_type});
}

// ERROR if Value has no type
TEST_F(MeasurementSeriesTest, ValueKindRejectStruct) {
  testonly::MockTestStep step;
  testonly::MockMeasurementSeries series(&step);
  series.DelegateToReal();
  google::protobuf::Value struct_type;
  *struct_type.mutable_struct_value() = google::protobuf::Struct();
  EXPECT_CALL(step, AddError(absl::string_view(kSympProceduralErr), _, _))
      .Times(1);
  series.AddElementWithValues(struct_type, {struct_type});
}

TEST_F(MeasurementSeriesTest, AddElementWithValidValues) {
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
TEST_F(MeasurementSeriesTest, AddElementWithValuesFailAfterEnded) {
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
}  // namespace ocpdiag
