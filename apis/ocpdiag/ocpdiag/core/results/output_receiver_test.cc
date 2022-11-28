// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/output_receiver.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "ocpdiag/core/results/results.h"
#include "ocpdiag/core/results/results.pb.h"
#include "ocpdiag/core/results/results_model.pb.h"
#include "ocpdiag/core/testing/parse_text_proto.h"
#include "ocpdiag/core/testing/status_matchers.h"

namespace ocpdiag::results {
namespace {

using ::ocpdiag::testing::ParseTextProtoOrDie;

TEST(OutputReceiverTest, Nominal) {
  OutputReceiver output;
  {
    TestRun test_run("Test", output.artifact_writer());
    test_run.LogInfo("Test log");
  }

  // Check that the output model is populated.
  EXPECT_FALSE(output.test_run().start().name().empty());
  EXPECT_FALSE(output.test_run().end().name().empty());

  // Iterate through the raw results.
  int count = 0;
  for (auto unused_artifact : output.raw()) count++;
  EXPECT_EQ(count, 3);
}

TEST(OutputModelTest, AddOutputArtifact_TestRunStart) {
  ocpdiag_results_pb::TestRunModel test_run;
  ASSERT_OK(
      AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                          test_run_artifact { test_run_start { name: "hi" } }
                        )pb")));
  EXPECT_FALSE(test_run.start().name().empty());
}

TEST(OutputModelTest, AddOutputArtifact_TestRunEnd) {
  ocpdiag_results_pb::TestRunModel test_run;
  ASSERT_OK(
      AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                          test_run_artifact { test_run_end { name: "hi" } }
                        )pb")));
  EXPECT_FALSE(test_run.end().name().empty());
}

TEST(OutputModelTest, AddOutputArtifact_Log) {
  ocpdiag_results_pb::TestRunModel test_run;
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
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_run_artifact { log { severity: FATAL } }
                              )pb")));
  EXPECT_EQ(test_run.logs().debug().size(), 1);
  EXPECT_EQ(test_run.logs().info().size(), 1);
  EXPECT_EQ(test_run.logs().warning().size(), 1);
  EXPECT_EQ(test_run.logs().error().size(), 1);
  EXPECT_EQ(test_run.logs().fatal().size(), 1);
}

TEST(OutputModelTest, AddOutputArtifact_Tag) {
  ocpdiag_results_pb::TestRunModel test_run;
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_run_artifact { tag {} }
                              )pb")));
  EXPECT_EQ(test_run.tags().size(), 1);
}

TEST(OutputModelTest, AddOutputArtifact_Error) {
  ocpdiag_results_pb::TestRunModel test_run;
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_run_artifact { error {} }
                              )pb")));
  EXPECT_EQ(test_run.errors().size(), 1);
}

TEST(OutputModelTest, AddOutputArtifact_TestStep) {
  ocpdiag_results_pb::TestRunModel test_run;
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_step_artifact {
                                  test_step_id: "1"
                                  test_step_start { name: "step1" }
                                }
                              )pb")));
  ASSERT_TRUE(test_run.test_steps().contains("1"));
  const ocpdiag_results_pb::TestStepModel& test_step =
      test_run.test_steps().at("1");
  EXPECT_FALSE(test_step.start().name().empty());

  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_step_artifact {
                                  test_step_id: "1"
                                  test_step_end { name: "step1" }
                                }
                              )pb")));
  EXPECT_FALSE(test_step.end().name().empty());

  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_step_artifact {
                                  test_step_id: "1"
                                  diagnosis {}
                                }
                              )pb")));
  EXPECT_EQ(test_step.diagnoses().size(), 1);

  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_step_artifact {
                                  test_step_id: "1"
                                  file {}
                                }
                              )pb")));
  EXPECT_EQ(test_step.files().size(), 1);

  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_step_artifact {
                                  test_step_id: "1"
                                  error {}
                                }
                              )pb")));
  EXPECT_EQ(test_step.errors().size(), 1);

  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_step_artifact {
                                  test_step_id: "1"
                                  extension {}
                                }
                              )pb")));
  EXPECT_EQ(test_step.artifact_extensions().size(), 1);

  // Add a measurement.
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_step_artifact {
                                  test_step_id: "1"
                                  measurement {}
                                }
                              )pb")));
  EXPECT_EQ(test_step.measurements().size(), 1);

  // Test starting/ending a measurement series with a single element.
  ASSERT_OK(AddOutputArtifact(
      test_run, ParseTextProtoOrDie(R"pb(
        test_step_artifact {
          test_step_id: "1"
          measurement_series_start { measurement_series_id: "0" }
        }
      )pb")));
  EXPECT_EQ(test_step.measurement_series().size(), 1);
  ASSERT_TRUE(test_step.measurement_series().contains("0"));
  EXPECT_FALSE(test_step.measurement_series()
                   .at("0")
                   .start()
                   .measurement_series_id()
                   .empty());

  ASSERT_OK(
      AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                          test_step_artifact {
                            test_step_id: "1"
                            measurement_element { measurement_series_id: "0" }
                          }
                        )pb")));
  EXPECT_EQ(
      test_step.measurement_series().at("0").measurement_elements().size(), 1);

  ASSERT_OK(AddOutputArtifact(
      test_run, ParseTextProtoOrDie(R"pb(
        test_step_artifact {
          test_step_id: "1"
          measurement_series_end { measurement_series_id: "0" }
        }
      )pb")));
  EXPECT_FALSE(test_step.measurement_series()
                   .at("0")
                   .end()
                   .measurement_series_id()
                   .empty());

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
  ASSERT_OK(AddOutputArtifact(test_run, ParseTextProtoOrDie(R"pb(
                                test_step_artifact {
                                  test_step_id: "1"
                                  log { severity: FATAL }
                                }
                              )pb")));
  EXPECT_EQ(test_step.logs().debug().size(), 1);
  EXPECT_EQ(test_step.logs().info().size(), 1);
  EXPECT_EQ(test_step.logs().warning().size(), 1);
  EXPECT_EQ(test_step.logs().error().size(), 1);
  EXPECT_EQ(test_step.logs().fatal().size(), 1);
}

}  // namespace
}  // namespace ocpdiag::results
