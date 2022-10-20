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
#include "absl/flags/reflection.h"
#include "absl/log/check.h"
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
#include "ocpdiag/core/results/recordio_iterator.h"
#include "ocpdiag/core/results/results.pb.h"
#include "ocpdiag/core/testing/parse_text_proto.h"
#include "ocpdiag/core/testing/proto_matchers.h"
#include "ocpdiag/core/testing/status_matchers.h"

namespace ocpdiag {
namespace results {
namespace internal {

using ::ocpdiag::testing::EqualsProto;
using ::ocpdiag::testing::ParseTextProtoOrDie;
using ::ocpdiag::testing::Partially;
using ::ocpdiag::testing::StatusIs;
using ::testing::_;
using ::testing::Contains;
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

rpb::OutputArtifact ParseJsonOrDie(absl::string_view json) {
  rpb::OutputArtifact got;
  CHECK_OK(AsAbslStatus(google::protobuf::util::JsonStringToMessage(json, &got)));
  return got;
}

class ResultsTestBase : public ::testing::Test {
 protected:
  ResultsTestBase()
      : output_filename_(absl::StrFormat(
            "%s/ocpdiag_results_test", std::filesystem::temp_directory_path())) {
    if (std::filesystem::exists(output_filename_))
      CHECK(std::filesystem::remove(output_filename_));

    absl::StatusOr<int> fd = OpenAndGetDescriptor(output_filename_);
    CHECK_OK(fd.status());

    writer_ = std::make_unique<ArtifactWriter>(*fd, &json_out_);
  }

  void IterateJsonOutput(std::function<void(rpb::OutputArtifact)> callback) {
    std::string line;
    while (true) {
      std::getline(json_out_, line);
      if (line.empty()) break;
      (void)callback(ParseJsonOrDie(line));
    }
  }

  // Finds a particular artifact in the results output. If more than one
  // artifacts satisfy the condition, only the last one is returned.
  absl::StatusOr<rpb::OutputArtifact> FindArtifact(
      std::function<bool(const rpb::OutputArtifact &)> condition) {
    std::optional<rpb::OutputArtifact> found;
    IterateJsonOutput([&](rpb::OutputArtifact artifact) {
      if (condition(artifact)) found = artifact;
    });
    if (!found.has_value()) return absl::NotFoundError("");
    return std::move(*found);
  }

  // Like FindArtifact(), only it returns all artifacts that match.
  std::vector<rpb::OutputArtifact> FindAllArtifacts(
      std::function<bool(const rpb::OutputArtifact &)> condition) {
    std::vector<rpb::OutputArtifact> found;
    IterateJsonOutput([&](rpb::OutputArtifact artifact) {
      if (condition(artifact)) found.push_back(artifact);
    });
    return found;
  }

  absl::FlagSaver flag_saver_;
  std::string output_filename_;
  std::stringstream json_out_;
  std::unique_ptr<ArtifactWriter> writer_;
};

// Test fixture for TestRun
using TestRunTest = ResultsTestBase;

TEST_F(TestRunTest, StartAndRegisterInfos) {
  TestRun test("TestRunTest", *writer_);
  ASSERT_FALSE(test.Started());
  DutInfo di("host");
  DutInfo di2("host2");
  HwRecord hw = di.AddHardware(ParseTextProtoOrDie(kHwRegistered));
  SwRecord sw = di.AddSoftware(ParseTextProtoOrDie(kSwRegistered));
  di.AddPlatformInfo("plugin:something");

  std::vector<ocpdiag::results::DutInfo> dutinfos;
  dutinfos.emplace_back(std::move(di));
  dutinfos.emplace_back(std::move(di2));
  google::protobuf::Empty params;
  test.StartAndRegisterInfos(dutinfos, params);
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

  // Read the test run start artifact and compare against expectations.
  ASSERT_OK_AND_ASSIGN(
      auto got, FindArtifact([&](rpb::OutputArtifact artifact) {
        return artifact.test_run_artifact().has_test_run_start();
      }));
  EXPECT_THAT(got, Partially(EqualsProto(want)));
  EXPECT_NE(got.test_run_artifact().test_run_start().version(), "");
  EXPECT_TRUE(test.Started());

  // Now check the recordIO fileoutput. We validate this once and use the json
  // output for the rest of the tests.
  got.Clear();
  RecordIoIterator<rpb::OutputArtifact> iter(output_filename_);
  for (; iter; ++iter)
    if (iter->test_run_artifact().has_test_run_start()) got = *iter;
  EXPECT_THAT(got, Partially(EqualsProto(want)));
}

TEST_F(TestRunTest, EndBeforeStart) {
  TestRun test("TestRunTest", *writer_);
  ASSERT_FALSE(test.Ended());
  rpb::TestResult result = test.End();
  // Expect NOT_APPLICABLE since test was never started
  ASSERT_EQ(result, rpb::TestResult::NOT_APPLICABLE);
}

TEST_F(TestRunTest, EndAfterStart) {
  TestRun test("TestRunTest", *writer_);
  ASSERT_FALSE(test.Ended());
  test.StartAndRegisterInfos({});
  rpb::TestResult result = test.End();
  ASSERT_TRUE(test.Ended());
  ASSERT_EQ(result, rpb::TestResult::PASS);
  ASSERT_EQ(test.Status(), rpb::TestStatus::COMPLETE);
  test.End();

  ASSERT_OK_AND_ASSIGN(auto got,
                       FindArtifact([&](rpb::OutputArtifact artifact) {
                         return artifact.test_run_artifact().has_test_run_end();
                       }));
  std::string want = R"pb(name: "TestRunTest" status: COMPLETE result: PASS)pb";
  EXPECT_THAT(got.test_run_artifact().test_run_end(),
              Partially(EqualsProto(want)));
}

TEST_F(TestRunTest, EndTwice) {
  TestRun test("TestRunTest", *writer_);
  test.End();
  test.End();

  // We should only find one "end" artifact.
  int got_count = 0;
  IterateJsonOutput([&](rpb::OutputArtifact a) {
    if (a.test_run_artifact().has_test_run_end()) got_count++;
  });
  EXPECT_EQ(got_count, 1);
}

// Expect default NOT_APPLICABLE:UNKNOWN
TEST_F(TestRunTest, ResultCalcDefaults) {
  TestRun test("TestRunTest", *writer_);
  EXPECT_EQ(rpb::TestStatus_Name(test.Status()),
            rpb::TestStatus_Name(rpb::TestStatus::UNKNOWN));
  EXPECT_EQ(rpb::TestResult_Name(test.Result()),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
}

// Expect NOT_APPLICABLE:SKIPPED if skipped and no errors emitted
TEST_F(TestRunTest, ResultCalcSkip) {
  TestRun test("TestRunTest", *writer_);
  rpb::TestResult result = test.Skip();
  EXPECT_EQ(rpb::TestStatus_Name(test.Status()),
            rpb::TestStatus_Name(rpb::TestStatus::SKIPPED));
  EXPECT_EQ(rpb::TestResult_Name(test.Result()),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
  EXPECT_EQ(rpb::TestResult_Name(result),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
}

// Expect NOT_APPLICABLE:ERROR even if skip called after error
TEST_F(TestRunTest, ResultCalcErrorThenSkip) {
  TestRun test("TestRunTest", *writer_);
  test.AddError("", "");
  rpb::TestResult result = test.Skip();
  EXPECT_EQ(rpb::TestStatus_Name(test.Status()),
            rpb::TestStatus_Name(rpb::TestStatus::ERROR));
  EXPECT_EQ(rpb::TestResult_Name(test.Result()),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
  EXPECT_EQ(rpb::TestResult_Name(result),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
}

// Expect PASS:COMPLETE if started and no diags/errors emitted
TEST_F(TestRunTest, ResultCalcPass) {
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({});
  rpb::TestResult result = test.End();
  EXPECT_EQ(rpb::TestStatus_Name(test.Status()),
            rpb::TestStatus_Name(rpb::TestStatus::COMPLETE));
  EXPECT_EQ(rpb::TestResult_Name(test.Result()),
            rpb::TestResult_Name(rpb::TestResult::PASS));
  EXPECT_EQ(rpb::TestResult_Name(result),
            rpb::TestResult_Name(rpb::TestResult::PASS));
}

// Expect FAIL:COMPLETE if started and fail diag emitted
TEST_F(TestRunTest, ResultCalcAddFailDiag) {
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("");
  step->AddDiagnosis(rpb::Diagnosis::FAIL, "", "", {});
  rpb::TestResult result = test.End();
  EXPECT_EQ(rpb::TestStatus_Name(test.Status()),
            rpb::TestStatus_Name(rpb::TestStatus::COMPLETE));
  EXPECT_EQ(rpb::TestResult_Name(test.Result()),
            rpb::TestResult_Name(rpb::TestResult::FAIL));
  EXPECT_EQ(rpb::TestResult_Name(result),
            rpb::TestResult_Name(rpb::TestResult::FAIL));
}

// Expect NOT_APPLICABLE:ERROR if error artifact emitted from TestRun
TEST_F(TestRunTest, ResultCalcTestRunAddError) {
  TestRun test("TestRunTest", *writer_);
  test.AddError("", "");
  rpb::TestResult result = test.End();
  EXPECT_EQ(rpb::TestStatus_Name(test.Status()),
            rpb::TestStatus_Name(rpb::TestStatus::ERROR));
  EXPECT_EQ(rpb::TestResult_Name(test.Result()),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
  EXPECT_EQ(rpb::TestResult_Name(result),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
}

// Expect NOT_APPLICABLE:ERROR if error artifact emitted from TestStep
TEST_F(TestRunTest, ResultCalcTestStepAddError) {
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("");
  step->AddError("", "", {});
  rpb::TestResult result = test.End();
  EXPECT_EQ(rpb::TestStatus_Name(test.Status()),
            rpb::TestStatus_Name(rpb::TestStatus::ERROR));
  EXPECT_EQ(rpb::TestResult_Name(test.Result()),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
  EXPECT_EQ(rpb::TestResult_Name(result),
            rpb::TestResult_Name(rpb::TestResult::NOT_APPLICABLE));
}

TEST_F(TestRunTest, AddError) {
  TestRun test("TestRunTest", *writer_);
  test.AddError("symptom", "msg");
  test.End();

  // Compare to what we expect (ignoring timestamp)
  std::string want =
      R"pb(test_run_artifact { error { symptom: "symptom" msg: "msg" } })pb";
  auto errors = FindAllArtifacts(
      [&](rpb::OutputArtifact a) { return a.test_run_artifact().has_error(); });
  EXPECT_THAT(errors, Contains(Partially(EqualsProto(want))));
}

TEST_F(TestRunTest, AddTag) {
  TestRun test("TestRunTest", *writer_);
  test.AddTag("Guten Tag");
  test.End();
  // Compare to what we expect (ignoring timestamp)
  std::string want =
      absl::StrFormat(R"pb(test_run_artifact { tag { tag: "Guten Tag" } })pb");
  auto tags = FindAllArtifacts(
      [&](rpb::OutputArtifact a) { return a.test_run_artifact().has_tag(); });
  EXPECT_THAT(tags, Contains(Partially(EqualsProto(want))));
}

TEST_F(TestRunTest, LogDebug) {
  TestRun test("TestRunTest", *writer_);
  test.LogDebug("m");
  test.End();
  auto logs = FindAllArtifacts([&](rpb::OutputArtifact a) {
    return a.test_run_artifact().log().severity() == rpb::Log::DEBUG;
  });
  std::string want =
      R"pb(test_run_artifact { log { severity: DEBUG text: "m" } })pb";
  EXPECT_THAT(logs, Contains(Partially(EqualsProto(want))));
}

TEST_F(TestRunTest, LogInfo) {
  TestRun test("TestRunTest", *writer_);
  test.LogInfo("m");
  test.End();
  auto logs = FindAllArtifacts([&](rpb::OutputArtifact a) {
    return a.test_run_artifact().log().severity() == rpb::Log::INFO;
  });
  std::string want =
      R"pb(test_run_artifact { log { severity: INFO text: "m" } })pb";
  EXPECT_THAT(logs, Contains(Partially(EqualsProto(want))));
}

TEST_F(TestRunTest, LogWarn) {
  TestRun test("TestRunTest", *writer_);
  test.LogWarn("m");
  test.End();
  auto logs = FindAllArtifacts([&](rpb::OutputArtifact a) {
    return a.test_run_artifact().log().severity() == rpb::Log::WARNING;
  });
  std::string want =
      R"pb(test_run_artifact { log { severity: WARNING text: "m" } })pb";
  EXPECT_THAT(logs, Contains(Partially(EqualsProto(want))));
}

TEST_F(TestRunTest, LogError) {
  TestRun test("TestRunTest", *writer_);
  test.LogError("m");
  test.End();
  auto logs = FindAllArtifacts([&](rpb::OutputArtifact a) {
    return a.test_run_artifact().log().severity() == rpb::Log::ERROR;
  });
  std::string want =
      R"pb(test_run_artifact { log { severity: ERROR text: "m" } })pb";
  EXPECT_THAT(logs, Contains(Partially(EqualsProto(want))));
}

TEST_F(TestRunTest, LogFatal) {
  TestRun test("TestRunTest", *writer_);
  test.LogFatal("m");
  test.End();
  auto logs = FindAllArtifacts([&](rpb::OutputArtifact a) {
    return a.test_run_artifact().log().severity() == rpb::Log::FATAL;
  });
  std::string want =
      R"pb(test_run_artifact { log { severity: FATAL text: "m" } })pb";
  EXPECT_THAT(logs, Contains(Partially(EqualsProto(want))));
  //
}

// Test fixture for TestStep
using TestStepTest = ResultsTestBase;
using TestStepDeathTest = TestStepTest;

TEST_F(TestStepTest, Begin) {
  // This test also confirms that the readable stream from the child object
  // (TestStep) writes to the same buffer as the parent (TestRun), as expected.
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({});
  test.BeginTestStep("TestStepTest");
  test.End();
  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([&](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_test_step_start();
                       }));
  EXPECT_THAT(
      got, Partially(EqualsProto(R"pb(test_step_artifact {
                                        test_step_start { name: "TestStepTest" }
                                        test_step_id: "0"
                                      })pb")));
}

TEST(TestStepDeathTest, BeginWithParentNotStarted) {
  ArtifactWriter writer(-1, nullptr);
  TestRun test("Begin", writer);
  ASSERT_DEATH(test.BeginTestStep("invalid"), "");
}

TEST_F(TestStepTest, AddDiagnosis) {
  DutInfo dut_info;
  HwRecord hw = dut_info.AddHardware(ParseTextProtoOrDie(kHwRegistered));
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({dut_info});
  auto step = test.BeginTestStep("AddDiagnosis");
  step->AddDiagnosis(rpb::Diagnosis::PASS, "symptom", "add diag success", {hw});
  test.End();

  // Compare to what we expect (ignoring timestamp)
  ASSERT_OK_AND_ASSIGN(auto got,
                       FindArtifact([&](rpb::OutputArtifact artifact) {
                         return artifact.test_step_artifact().has_diagnosis();
                       }));
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
                                     hw.Data().hardware_info_id(), step->Id());
  EXPECT_THAT(got, Partially(EqualsProto(want)));
}

TEST_F(TestStepTest, AddDiagnosisHwUnregistered) {
  DutInfo dut_info;
  HwRecord hw = dut_info.AddHardware(ParseTextProtoOrDie(kHwNotRegistered));
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("AddDiagnosis");
  step->AddDiagnosis(rpb::Diagnosis::FAIL, "symptom", "add diag fail", {hw});

  std::vector<std::string> wants;

  // Expect test step start.
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          test_step_id: "%s"
          test_step_start {}
        }
      )pb",
      step->Id());
  wants.push_back(want);

  // Expect Error log artifact.
  want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          error { symptom: "%s" }
          test_step_id: "%s"
        }
      )pb",
      kSympUnregHw, step->Id());
  wants.push_back(want);

  // Then expect diag result with missing hardware_info_id
  want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          diagnosis { symptom: "symptom" type: FAIL msg: "add diag fail" }
          test_step_id: "%s"
        }
      )pb",
      step->Id());
  wants.push_back(want);
  test.End();

  auto gots = FindAllArtifacts(
      [](rpb::OutputArtifact a) { return a.has_test_step_artifact(); });
  ASSERT_EQ(wants.size(), gots.size());
  for (size_t i = 0; i < wants.size(); i++)
    EXPECT_THAT(gots[i], Partially(EqualsProto(wants[i])));
}

TEST_F(TestStepTest, AddError) {
  DutInfo dut_info;
  SwRecord swrec = dut_info.AddSoftware(ParseTextProtoOrDie(kSwRegistered));
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({dut_info});
  auto step = test.BeginTestStep("AddError");
  step->AddError("symptom", "add error success", {swrec});
  test.End();

  // Compare to what we expect (ignoring timestamp)
  std::string want =
      absl::StrFormat(R"pb(
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
                      swrec.Data().software_info_id(), step->Id());
  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([&](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_error();
                       }));
  EXPECT_THAT(got, Partially(EqualsProto(want)));
  EXPECT_EQ(step->Status(), rpb::TestStatus::ERROR);
  EXPECT_EQ(test.Status(), rpb::TestStatus::ERROR);
}

TEST_F(TestStepTest, AddErrorSwUnregistered) {
  DutInfo dut_info;
  SwRecord swrec = dut_info.AddSoftware(ParseTextProtoOrDie(kSwNotRegistered));
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("AddError");
  step->AddError("symptom", "add error fail", {swrec});
  test.End();

  // Expect test step start.
  std::vector<std::string> wants;
  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          test_step_id: "%s"
          test_step_start {}
        }
      )pb",
      step->Id());
  wants.push_back(want);

  // Expect Error log artifact first.
  want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          error { symptom: "%s" }
          test_step_id: "%s"
        }
      )pb",
      kSympUnregSw, step->Id());
  wants.push_back(want);

  // Then expect error result with missing software_info_id
  want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          error { symptom: "symptom" msg: "add error fail" }
          test_step_id: "%s"
        }
      )pb",
      step->Id());
  wants.push_back(want);

  auto gots = FindAllArtifacts(
      [](rpb::OutputArtifact a) { return a.has_test_step_artifact(); });
  ASSERT_EQ(wants.size(), gots.size());
  for (size_t i = 0; i < wants.size(); i++) {
    EXPECT_THAT(gots[i], Partially(EqualsProto(wants[i])));
  }
  ASSERT_EQ(step->Status(), rpb::TestStatus::ERROR);
}

TEST_F(TestStepTest, AddMeasurement) {
  DutInfo dut_info;
  HwRecord hw = dut_info.AddHardware(ParseTextProtoOrDie(kHwRegistered));
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({dut_info});
  auto step = test.BeginTestStep("AddMeasurement");

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

  step->AddMeasurement(info, elem, &hw);
  test.End();

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
      hw.Data().hardware_info_id(), now.seconds(), now.nanos(), step->Id());
  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([&](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_measurement();
                       }));
  EXPECT_THAT(got, Partially(EqualsProto(want)));
}

// Expect Error artifact if MeasurementElement is mal-formed (Struct kind not
// allowed)
TEST_F(TestStepTest, AddMeasurementFail) {
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("step");
  rpb::MeasurementElement elem;
  *elem.mutable_value()->mutable_struct_value() = google::protobuf::Struct();
  step->AddMeasurement({}, elem, nullptr);
  test.End();

  // This fails the test if an error was not found.
  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([&](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_error();
                       }));
}

class ValidateMeasElemTest : public ResultsTestBase {
 protected:
  ValidateMeasElemTest() {
    test_ = std::make_unique<TestRun>("TestRunTest", *writer_);

    test_->StartAndRegisterInfos({});
    step_ = test_->BeginTestStep("step");
  }

  std::unique_ptr<TestRun> test_;
  std::unique_ptr<TestStep> step_;
};

TEST_F(ValidateMeasElemTest, InvalidKind) {
  rpb::MeasurementElement elem;
  *elem.mutable_value()->mutable_struct_value() = google::protobuf::Struct();
  absl::Status status = TestStep::ValidateMeasurementElement(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(absl::StrContains(status.message(), "value of kind 'Struct'"));
}

TEST_F(ValidateMeasElemTest, InvalidRangeKind) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_bool_value(
      true);  // valid for values, but not range.
  elem.mutable_range()->mutable_maximum()->set_number_value(0);
  elem.mutable_range()->mutable_minimum()->set_number_value(0);
  absl::Status status = TestStep::ValidateMeasurementElement(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(absl::StrContains(status.message(), "value of kind 'bool'"));
}

TEST_F(ValidateMeasElemTest, InvalidRangeMinKind) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_number_value(0);
  elem.mutable_range()->mutable_maximum()->set_number_value(0);
  elem.mutable_range()->mutable_minimum()->set_bool_value(true);
  absl::Status status = TestStep::ValidateMeasurementElement(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(absl::StrContains(status.message(), "value of kind 'bool'"));
}

TEST_F(ValidateMeasElemTest, InvalidRangeMaxKind) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_number_value(0);
  elem.mutable_range()->mutable_maximum()->set_null_value({});
  elem.mutable_range()->mutable_minimum()->set_number_value(0);
  absl::Status status = TestStep::ValidateMeasurementElement(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(absl::StrContains(status.message(), "value of kind 'NullValue'"));
}

TEST_F(ValidateMeasElemTest, ValuesMismatch) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_bool_value(true);
  elem.mutable_valid_values()->mutable_values()->Add(google::protobuf::Value{});
  absl::Status status = TestStep::ValidateMeasurementElement(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(
      absl::StrContains(status.message(), "value of kind 'kind not set'"));
}

TEST_F(ValidateMeasElemTest, RangeMinMismatch) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_string_value("");
  elem.mutable_range()->mutable_maximum()->set_string_value("");
  elem.mutable_range()->mutable_minimum()->set_number_value(0);
  absl::Status status = TestStep::ValidateMeasurementElement(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(
      absl::StrContains(status.message(), "'double' does not equal 'string'"));
}

TEST_F(ValidateMeasElemTest, RangeMaxMismatch) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_string_value("");
  elem.mutable_range()->mutable_maximum()->set_number_value(0);
  elem.mutable_range()->mutable_minimum()->set_string_value("");
  absl::Status status = TestStep::ValidateMeasurementElement(elem);
  ASSERT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_TRUE(
      absl::StrContains(status.message(), "'double' does not equal 'string'"));
}

TEST_F(ValidateMeasElemTest, RangeMaxEmpty) {
  rpb::MeasurementElement elem;
  elem.mutable_value()->set_string_value("");
  elem.mutable_range()->mutable_minimum()->set_string_value("");
  EXPECT_OK(TestStep::ValidateMeasurementElement(elem));
}

TEST_F(TestStepTest, AddMeasurementWithNullptr) {
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("AddMeasurement");

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

  step->AddMeasurement(info, elem, nullptr);
  test.End();

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
      now.seconds(), now.nanos(), step->Id());
  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([&](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_measurement();
                       }));
  EXPECT_THAT(got, Partially(EqualsProto(want)));
}

TEST_F(TestStepTest, AddMeasurementHwUnregistered) {
  DutInfo dut_info;
  HwRecord hw = dut_info.AddHardware(ParseTextProtoOrDie(kHwRegistered));
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("AddMeasurement");

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

  step->AddMeasurement(info, elem, &hw);
  test.End();

  std::vector<std::string> wants;
  wants.push_back(absl::StrFormat(
      R"pb(
        test_step_artifact {
          test_step_id: "%s"
          test_step_start {}
        }
      )pb",
      step->Id()));

  // Expect Error Log artifact
  wants.push_back(absl::StrFormat(
      R"pb(
        test_step_artifact {
          error { symptom: "%s" }
          test_step_id: "%s"
        }
      )pb",
      kSympUnregHw, step->Id()));

  wants.push_back(absl::StrFormat(
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
      now.seconds(), now.nanos(), step->Id()));

  auto gots = FindAllArtifacts(
      [&](rpb::OutputArtifact a) { return a.has_test_step_artifact(); });
  ASSERT_EQ(wants.size(), gots.size());
  for (size_t i = 0; i < wants.size(); i++)
    EXPECT_THAT(gots[i], Partially(EqualsProto(wants[i])));
}

// Test AddFile on local CWD file, no file copy expected.
TEST_F(TestStepTest, AddFile) {
  // Make sure copy methods are not called.
  auto fh = std::make_unique<MockFileHandler>();
  EXPECT_CALL(*fh, CopyLocalFile(_, _)).Times(0);
  EXPECT_CALL(*fh, CopyRemoteFile(_)).Times(0);

  TestRun test("AddFile", *writer_, std::move(fh));
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("AddFile");

  std::error_code err;
  std::string cwd = std::filesystem::current_path(err);
  ASSERT_FALSE(err);

  std::string output_path = absl::StrCat(cwd, "cwdfile.LOG");
  rpb::File file;
  file.set_upload_as_name("upload name");
  file.set_output_path(output_path);
  file.set_description("description");
  file.set_content_type("content type");
  step->AddFile(file);
  test.End();

  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([&](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_file();
                       }));
  EXPECT_THAT(got, Partially(EqualsProto(absl::StrFormat(
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
                       output_path, step->Id()))));
}

// expect local file copy
TEST_F(TestStepTest, AddFileLocalCopy) {
  auto fh = std::make_unique<MockFileHandler>();
  EXPECT_CALL(*fh, CopyLocalFile(_, _)).WillOnce(Return(absl::OkStatus()));

  TestRun test("AddFile", *writer_, std::move(fh));
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("AddFile");

  // Set a local path not in test dir, so expect a file copy.
  rpb::File file;
  file.set_output_path("/tmp/temporary_file_testing_copies");
  step->AddFile(file);
  test.End();

  // Skip proto output comparison, this is tested already
}

TEST_F(TestStepTest, AddFileFail) {
  auto fh = std::make_unique<internal::MockFileHandler>();
  EXPECT_CALL(*fh, CopyLocalFile(_, _))
      .WillOnce(Return(absl::UnknownError("")));

  TestRun test("AddFile", *writer_, std::move(fh));
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("AddFile");
  rpb::File file;
  file.set_output_path("/tmp/data/file");
  step->AddFile(file);
  test.End();

  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          error { symptom: "ocpdiag-internal-error" }
          test_step_id: "%s"
        }
      )pb",
      step->Id());
  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([&](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_error();
                       }));
  EXPECT_THAT(got, Partially(EqualsProto(want)));
}

TEST_F(TestStepTest, AddFileRemote) {
  auto fh = std::make_unique<MockFileHandler>();
  EXPECT_CALL(*fh, CopyRemoteFile(_)).WillOnce(Return(absl::OkStatus()));

  TestRun test("AddFileRemote", *writer_, std::move(fh));
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("AddFileRemote");

  rpb::File file;
  file.set_upload_as_name("upload name");
  // mimic absolute path on remote node.
  file.set_output_path("/tmp/data/file");
  file.set_description("description");
  file.set_content_type("content type");
  file.set_node_address("node_address");
  step->AddFile(file);
}

TEST_F(TestStepTest, AddFileRemoteFail) {
  auto fh = std::make_unique<MockFileHandler>();
  EXPECT_CALL(*fh, CopyRemoteFile(_)).WillOnce(Return(absl::UnknownError("")));

  TestRun test("AddFileRemote", *writer_, std::move(fh));
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("AddFileRemote");

  rpb::File file;
  file.set_upload_as_name("upload name");
  // mimic absolute path on remote node.
  file.set_output_path("/tmp/data/file");
  file.set_description("description");
  file.set_content_type("content type");
  file.set_node_address("node_address");
  step->AddFile(file);
  test.End();

  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          error { symptom: "ocpdiag-internal-error" }
          test_step_id: "%s"
        }
      )pb",
      step->Id());
  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([&](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_error();
                       }));
  EXPECT_THAT(got, Partially(EqualsProto(want)));
}

TEST_F(TestStepTest, AddArtifactExtension) {
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("AddArtifactExtension");

  google::protobuf::Empty extension;
  step->AddArtifactExtension("test extension", extension);
  test.End();

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
      extension.SerializeAsString(), step->Id());

  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([&](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_extension();
                       }));
  EXPECT_THAT(got, Partially(EqualsProto(want)));
}

TEST_F(TestStepTest, End) {
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("TestStepTest");

  ASSERT_FALSE(step->Ended());
  ASSERT_EQ(step->Status(), rpb::TestStatus::UNKNOWN);
  step->End();

  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          test_step_end { name: "TestStepTest" status: COMPLETE }
          test_step_id: "%s"
        }
        sequence_number: 0
      )pb",
      step->Id());
  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([&](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_test_step_end();
                       }));
  EXPECT_THAT(got, Partially(EqualsProto(want)));
  ASSERT_EQ(step->Status(), rpb::TestStatus::COMPLETE);
  ASSERT_TRUE(step->Ended());
}

TEST_F(TestStepTest, Skip) {
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("TestStepTest");

  ASSERT_EQ(step->Status(), rpb::TestStatus::UNKNOWN);
  step->Skip();
  ASSERT_EQ(step->Status(), rpb::TestStatus::SKIPPED);
  ASSERT_TRUE(step->Ended());
}

TEST_F(TestStepTest, LogDebug) {
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("TestStepTest");
  step->LogDebug("my house has termites, please debug it");
  test.End();

  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          log { severity: DEBUG text: "my house has termites, please debug it" }
          test_step_id: "%s"
        }
      )pb",
      step->Id());
  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([&](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_log();
                       }));
  EXPECT_THAT(got, Partially(EqualsProto(want)));
}

TEST_F(TestStepTest, LogInfo) {
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("TestStepTest");
  step->LogInfo("Here, have an info");
  test.End();

  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([&](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_log();
                       }));
  EXPECT_THAT(got, Partially(EqualsProto(absl::StrFormat(
                       R"pb(
                         test_step_artifact {
                           log { severity: INFO text: "Here, have an info" }
                           test_step_id: "%s"
                         }
                       )pb",
                       step->Id()))));
}

TEST_F(TestStepTest, LogWarn) {
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("TestStepTest");
  step->LogWarn("this test is brand new, never warn");
  test.End();

  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          log { severity: WARNING text: "this test is brand new, never warn" }
          test_step_id: "%s"
        }
      )pb",
      step->Id());
  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([&](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_log();
                       }));
  EXPECT_THAT(got, Partially(EqualsProto(want)));
}

TEST_F(TestStepTest, LogError) {
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("TestStepTest");
  step->LogError("to err is human");
  test.End();

  std::string want =
      absl::StrFormat(R"pb(
                        test_step_artifact {
                          log { severity: ERROR text: "to err is human" }
                          test_step_id: "%s"
                        }
                      )pb",
                      step->Id());
  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([&](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_log();
                       }));
  EXPECT_THAT(got, Partially(EqualsProto(want)));
}

TEST_F(TestStepTest, LogFatal) {
  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({});
  auto step = test.BeginTestStep("TestStepTest");
  step->LogFatal("What's my destiny? Fatal determine that");
  test.End();

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
                      step->Id());
  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([&](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_log();
                       }));
  EXPECT_THAT(got, Partially(EqualsProto(want)));
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
  for (std::future<void> &thread : threads) {
    thread.wait();
  }
  EXPECT_EQ(kWantCount, ids.size());
}

TEST_F(TestStepTest, BeginMeasurementSeriesHwUnregistered) {
  DutInfo dut_info;
  HwRecord hw = dut_info.AddHardware(ParseTextProtoOrDie(kHwNotRegistered));

  TestRun test("TestRunTest", *writer_);
  test.StartAndRegisterInfos({});  // no dut info registered
  auto step = test.BeginTestStep("TestStepTest");

  std::unique_ptr<MeasurementSeries> series =
      step->BeginMeasurementSeries(hw, ParseTextProtoOrDie(kMeasurementInfo));
  test.End();

  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          error { symptom: "%s" }
          test_step_id: "%s"
        }
      )pb",
      kSympUnregHw, step->Id());

  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([&](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_error();
                       }));
  EXPECT_THAT(got, Partially(EqualsProto(want)));
}

class MeasurementSeriesTest : public ResultsTestBase {
 protected:
  MeasurementSeriesTest() {
    hw_ = dut_info_.AddHardware(ParseTextProtoOrDie(kHwRegistered));

    test_ = std::make_unique<TestRun>("TestRunTest", *writer_);
    test_->StartAndRegisterInfos({dut_info_});
    step_ = test_->BeginTestStep("TestStepTest");
  }

  DutInfo dut_info_;
  HwRecord hw_;

  std::unique_ptr<TestRun> test_;
  std::unique_ptr<TestStep> step_;
};
using MeasurementSeriesDeathTest = MeasurementSeriesTest;

TEST_F(MeasurementSeriesTest, Begin) {
  std::string hw_id = hw_.Data().hardware_info_id();

  rpb::MeasurementInfo info =
      ParseTextProtoOrDie(absl::StrFormat(kMeasurementInfo, hw_id));
  std::unique_ptr<MeasurementSeries> series =
      step_->BeginMeasurementSeries(hw_, info);
  test_->End();

  std::string want = absl::StrFormat(
      R"pb(
        test_step_artifact {
          measurement_series_start { measurement_series_id: "%s" info { %s } }
          test_step_id: "%s"
        }
      )pb",
      series->Id(), absl::StrFormat(kMeasurementInfo, hw_id), step_->Id());

  ASSERT_OK_AND_ASSIGN(
      auto got, FindArtifact([&](rpb::OutputArtifact a) {
        return a.test_step_artifact().has_measurement_series_start();
      }));
  EXPECT_THAT(got, Partially(EqualsProto(want)));
}

// Asserts that a MeasurementSeries cannot be started with a step that already
// ended.
TEST_F(MeasurementSeriesDeathTest, BeginFailAfterStepEnd) {
  step_->End();
  ASSERT_DEATH(step_->BeginMeasurementSeries(hw_, rpb::MeasurementInfo()), "");
}

TEST_F(MeasurementSeriesTest, AddElementWithoutLimit) {
  auto series = step_->BeginMeasurementSeries(hw_, rpb::MeasurementInfo());

  google::protobuf::Value val;
  val.set_number_value(3.14);
  series->AddElement(val);
  test_->End();

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
      series->Id(), kStepIdDefault);

  ASSERT_OK_AND_ASSIGN(
      auto got, FindArtifact([&](rpb::OutputArtifact a) {
        return a.test_step_artifact().has_measurement_element();
      }));
  EXPECT_THAT(got, Partially(EqualsProto(want)));
}

TEST_F(MeasurementSeriesTest, AddElementWithRange) {
  auto series = step_->BeginMeasurementSeries(hw_, rpb::MeasurementInfo());

  google::protobuf::Value val, min, max;
  rpb::MeasurementElement::Range range;
  min.set_number_value(3.13);
  val.set_number_value(3.14);
  max.set_number_value(3.15);
  *range.mutable_minimum() = std::move(min);
  *range.mutable_maximum() = std::move(max);
  series->AddElementWithRange(val, range);
  test_->End();

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
      series->Id(), kStepIdDefault);

  ASSERT_OK_AND_ASSIGN(
      auto got, FindArtifact([&](rpb::OutputArtifact a) {
        return a.test_step_artifact().has_measurement_element();
      }));
  EXPECT_THAT(got, Partially(EqualsProto(want)));
}

// ERROR if Range used as limit for ListValue
TEST_F(MeasurementSeriesTest, ValueKindListWithRangeLimit) {
  auto series = step_->BeginMeasurementSeries(hw_, rpb::MeasurementInfo());

  google::protobuf::Value val, list;
  val.set_number_value(0);
  *list.mutable_list_value()->add_values() = std::move(val);
  series->AddElementWithRange(list, {});
  test_->End();

  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([&](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_error();
                       }));
  EXPECT_EQ(got.test_step_artifact().error().symptom(), kSympProceduralErr);
}

// ERROR if Value kinds don't match
TEST_F(MeasurementSeriesTest, ValueKindMismatch) {
  auto series = step_->BeginMeasurementSeries(hw_, rpb::MeasurementInfo());

  google::protobuf::Value number_val, string_val;
  number_val.set_number_value(0);
  string_val.set_string_value("zero");
  series->AddElementWithValues(number_val, {string_val});
  test_->End();

  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_error();
                       }));
  EXPECT_THAT(got, Partially(EqualsProto(absl::StrFormat(
                       R"pb(test_step_artifact { error { symptom: "%s" } })pb",
                       kSympProceduralErr))));
}

// ERROR if Value has no type
TEST_F(MeasurementSeriesTest, ValueKindNoType) {
  auto series = step_->BeginMeasurementSeries(hw_, rpb::MeasurementInfo());

  google::protobuf::Value no_type;
  series->AddElementWithValues(no_type, {no_type});
  test_->End();

  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_error();
                       }));
  EXPECT_THAT(got, Partially(EqualsProto(absl::StrFormat(
                       R"pb(test_step_artifact { error { symptom: "%s" } })pb",
                       kSympProceduralErr))));
}

// ERROR if Value has no type
TEST_F(MeasurementSeriesTest, ValueKindRejectStruct) {
  auto series = step_->BeginMeasurementSeries(hw_, rpb::MeasurementInfo());

  google::protobuf::Value struct_type;
  *struct_type.mutable_struct_value() = google::protobuf::Struct();
  series->AddElementWithValues(struct_type, {struct_type});
  test_->End();

  ASSERT_OK_AND_ASSIGN(auto got, FindArtifact([](rpb::OutputArtifact a) {
                         return a.test_step_artifact().has_error();
                       }));
  EXPECT_THAT(got, Partially(EqualsProto(absl::StrFormat(
                       R"pb(test_step_artifact { error { symptom: "%s" } })pb",
                       kSympProceduralErr))));
}

TEST_F(MeasurementSeriesTest, AddElementWithValidValues) {
  auto series = step_->BeginMeasurementSeries(hw_, rpb::MeasurementInfo());

  google::protobuf::Value val;
  val.set_number_value(3.14);
  series->AddElementWithValues(val, {val});
  test_->End();

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
      series->Id(), kStepIdDefault);

  ASSERT_OK_AND_ASSIGN(
      auto got, FindArtifact([&](rpb::OutputArtifact a) {
        return a.test_step_artifact().has_measurement_element();
      }));
  EXPECT_THAT(got, Partially(EqualsProto(want)));
}

// Ensures MeasurementElements cannot be added to a series after it has ended.
TEST_F(MeasurementSeriesTest, AddElementWithValuesFailAfterEnded) {
  auto series = step_->BeginMeasurementSeries(hw_, rpb::MeasurementInfo());
  series->End();

  google::protobuf::Value val;
  rpb::MeasurementElement::Range range;
  range.mutable_maximum()->set_number_value(0);
  range.mutable_minimum()->set_number_value(0);
  val.set_number_value(0);
  series->AddElementWithValues(val, {val});
  series->AddElementWithRange(val, range);
  test_->End();

  auto got = FindArtifact([&](rpb::OutputArtifact a) {
    return a.test_step_artifact().has_measurement_element();
  });
  // We should not find any measurement elements.
  EXPECT_FALSE(got.status().ok());
}

TEST_F(MeasurementSeriesTest, End) {
  auto series = step_->BeginMeasurementSeries(hw_, rpb::MeasurementInfo());

  ASSERT_FALSE(series->Ended());
  series->End();
  ASSERT_TRUE(series->Ended());
  test_->End();

  std::string want = absl::StrFormat(R"pb(
                                       test_step_artifact {
                                         measurement_series_end {
                                           measurement_series_id: "%s"
                                           total_measurement_count: 0
                                         }
                                         test_step_id: "%s"
                                       }
                                     )pb",
                                     series->Id(), kStepIdDefault);
  ASSERT_OK_AND_ASSIGN(
      auto got, FindArtifact([&](rpb::OutputArtifact a) {
        return a.test_step_artifact().has_measurement_series_end();
      }));
  EXPECT_THAT(got, Partially(EqualsProto(want)));
}

}  // namespace internal
}  // namespace results
}  // namespace ocpdiag
