// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/output_model.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "ocpdiag/core/results/results.pb.h"
#include "ocpdiag/core/testing/parse_text_proto.h"
#include "ocpdiag/core/testing/status_matchers.h"

namespace ocpdiag::results {
namespace {

using ::ocpdiag::testing::ParseTextProtoOrDie;
using ::ocpdiag::results_pb::Log;

TEST(ResultsOutputParserTest, AddOutputArtifact_TestRunStart) {
  TestRunOutput test_run;
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_run_artifact { test_run_start {} }
                              )pb")));
  EXPECT_TRUE(test_run.start.has_value());
}

TEST(ResultsOutputParserTest, AddOutputArtifact_TestRunEnd) {
  TestRunOutput test_run;
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_run_artifact { test_run_end {} }
                              )pb")));
  EXPECT_TRUE(test_run.end.has_value());
}

TEST(ResultsOutputParserTest, AddOutputArtifact_Log) {
  TestRunOutput test_run;
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_run_artifact { log { severity: INFO } }
                              )pb")));
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_run_artifact { log { severity: WARNING } }
                              )pb")));
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_run_artifact { log { severity: DEBUG } }
                              )pb")));
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_run_artifact { log { severity: ERROR } }
                              )pb")));
  EXPECT_EQ(test_run.logs[Log::INFO].size(), 1);
  EXPECT_EQ(test_run.logs[Log::WARNING].size(), 1);
  EXPECT_EQ(test_run.logs[Log::DEBUG].size(), 1);
  EXPECT_EQ(test_run.logs[Log::ERROR].size(), 1);
}

TEST(ResultsOutputParserTest, AddOutputArtifact_Tag) {
  TestRunOutput test_run;
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_run_artifact { tag {} }
                              )pb")));
  EXPECT_EQ(test_run.tags.size(), 1);
}

TEST(ResultsOutputParserTest, AddOutputArtifact_Error) {
  TestRunOutput test_run;
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_run_artifact { error {} }
                              )pb")));
  EXPECT_EQ(test_run.errors.size(), 1);
}

TEST(ResultsOutputParserTest, AddOutputArtifact_TestStepStart) {
  TestRunOutput test_run;
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_step_artifact {
                                  test_step_id: "1"
                                  test_step_start { name: "step1" }
                                }
                              )pb")));
  EXPECT_TRUE(test_run.steps["1"].start.has_value());

  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_step_artifact {
                                  test_step_id: "1"
                                  test_step_end {}
                                }
                              )pb")));
  EXPECT_TRUE(test_run.steps["1"].end.has_value());

  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_step_artifact {
                                  test_step_id: "1"
                                  diagnosis {}
                                }
                              )pb")));
  EXPECT_EQ(test_run.steps["1"].diagnoses.size(), 1);

  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_step_artifact {
                                  test_step_id: "1"
                                  file {}
                                }
                              )pb")));
  EXPECT_EQ(test_run.steps["1"].files.size(), 1);

  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_step_artifact {
                                  test_step_id: "1"
                                  error {}
                                }
                              )pb")));
  EXPECT_EQ(test_run.steps["1"].errors.size(), 1);

  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_step_artifact {
                                  test_step_id: "1"
                                  extension {}
                                }
                              )pb")));
  EXPECT_EQ(test_run.steps["1"].artifact_extensions.size(), 1);

  // Add a measurement.
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_step_artifact {
                                  test_step_id: "1"
                                  measurement {}
                                }
                              )pb")));
  EXPECT_EQ(test_run.steps["1"].measurements.size(), 1);

  // Test starting/ending a measurement series with a single element.
  ASSERT_OK(AddOutputArtifact(
      test_run, ParseTextProtoOrDie(R"pb(
        test_step_artifact {
          test_step_id: "1"
          measurement_series_start { measurement_series_id: "0" }
        }
      )pb")));
  EXPECT_EQ(test_run.steps["1"].measurement_series.size(), 1);
  EXPECT_TRUE(test_run.steps["1"].measurement_series.at("0").start.has_value());

  ASSERT_OK(
      AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                          test_step_artifact {
                            test_step_id: "1"
                            measurement_element { measurement_series_id: "0" }
                          }
                        )pb")));
  EXPECT_EQ(test_run.steps["1"]
                .measurement_series.at("0")
                .measurement_elements.size(),
            1);

  ASSERT_OK(AddOutputArtifact(
      test_run, ParseTextProtoOrDie(R"pb(
        test_step_artifact {
          test_step_id: "1"
          measurement_series_end { measurement_series_id: "0" }
        }
      )pb")));
  EXPECT_TRUE(test_run.steps["1"].measurement_series.at("0").end.has_value());

  // Test log messages on the test step.
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_step_artifact {
                                  test_step_id: "1"
                                  log { severity: INFO }
                                }
                              )pb")));
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_step_artifact {
                                  test_step_id: "1"
                                  log { severity: WARNING }
                                }
                              )pb")));
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_step_artifact {
                                  test_step_id: "1"
                                  log { severity: DEBUG }
                                }
                              )pb")));
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_step_artifact {
                                  test_step_id: "1"
                                  log { severity: ERROR }
                                }
                              )pb")));
  EXPECT_EQ(test_run.steps["1"].logs[Log::INFO].size(), 1);
  EXPECT_EQ(test_run.steps["1"].logs[Log::WARNING].size(), 1);
  EXPECT_EQ(test_run.steps["1"].logs[Log::DEBUG].size(), 1);
  EXPECT_EQ(test_run.steps["1"].logs[Log::ERROR].size(), 1);
}

}  // namespace
}  // namespace ocpdiag::results
