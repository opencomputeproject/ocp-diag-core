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

#include "meltan/core/examples/simple/simple_lib.h"

#include <cstddef>
#include <memory>
#include <regex>  //
#include <vector>

#include "google/protobuf/util/json_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/str_split.h"
#include "meltan/core/compat/status_converters.h"
#include "meltan/core/examples/simple/params.pb.h"
#include "meltan/lib/results/results.h"
#include "meltan/lib/results/results.pb.h"
#include "meltan/lib/testing/fake_test_run.h"
#include "meltan/lib/testing/proto_matchers.h"
#include "meltan/lib/testing/status_matchers.h"

namespace meltan {
namespace example {
namespace {

using ::meltan::AsAbslStatus;
using ::meltan::FakeTestRun;
using ::meltan::TestRunResultHandler;
using ::meltan::results::HwRecord;
using ::meltan::results::TestStep;
using ::meltan::testing::EqualsProto;
using ::meltan::testing::Partially;
using ::meltan::results_pb::HardwareInfo;
using ::meltan::results_pb::Measurement;
using ::meltan::results_pb::OutputArtifact;

TEST(TestAddAllMeasurementTypes, CheckAll) {
  // Set up.
  results::ResultApi api;
  TestRunResultHandler test_run_handler("fake_simple");
  FakeTestRun* test_run = test_run_handler.GetFakeTestRun();
  meltan::simple::Params params;
  meltan::results::DutInfo dut_info("FakeHost");
  HardwareInfo hw_info;
  HwRecord hw_record = dut_info.AddHardware(hw_info);
  std::vector<meltan::results::DutInfo> dutinfos;
  dutinfos.emplace_back(std::move(dut_info));
  test_run->StartAndRegisterInfos(dutinfos, params);
  absl::StatusOr<std::unique_ptr<TestStep>> step =
      api.BeginTestStep(test_run, "fake_step");
  ASSERT_OK(step);

  // Run AddAllTypeMeasurements.
  meltan::example::AddAllMeasurementTypes(step->get(), &hw_record);

  // Check measurements.
  std::vector<std::string> lines =
      absl::StrSplit(test_run_handler.GetJsonOut(), '\n');

  std::vector<Measurement> measurements;
  google::protobuf::util::JsonParseOptions json_parse_options;
  for (const std::string& line : lines) {
    if (line.empty()) {
      continue;
    }
    OutputArtifact artifact;
    ASSERT_OK(
        AsAbslStatus(JsonStringToMessage(line, &artifact, json_parse_options)));
    if (artifact.has_test_step_artifact() &&
        artifact.test_step_artifact().has_measurement()) {
      measurements.push_back(artifact.test_step_artifact().measurement());
    }
  }

  std::vector<std::string> expected_measurements{
      // Null measurements.
      R"pb(
        info { name: "null-measurement" unit: "null-measurement-unit" }
        element {
          measurement_series_id: "NOT_APPLICABLE"
          value { null_value: NULL_VALUE }
          valid_values {
            values { null_value: NULL_VALUE }
            values { null_value: NULL_VALUE }
            values { null_value: NULL_VALUE }
          }
        }
      )pb",
      // Number measurements.
      R"pb(
        info { name: "number-measurement" unit: "number-measurement-unit" }
        element {
          measurement_series_id: "NOT_APPLICABLE"
          value { number_value: 1.23 }
          range {
            maximum { number_value: 2.34 }
            minimum { number_value: 0.12 }
          }
        }
      )pb",
      R"pb(
        info { name: "number-measurement" unit: "number-measurement-unit" }
        element {
          measurement_series_id: "NOT_APPLICABLE"
          value { number_value: 1.23 }
          valid_values {
            values { number_value: 1.23 }
            values { number_value: 2.34 }
            values { number_value: 0.12 }
          }
        }
      )pb",
      // String measurements.
      R"pb(
        info { name: "string-measurement" unit: "string-measurement-unit" }
        element {
          measurement_series_id: "NOT_APPLICABLE"
          value { string_value: "version-1.23" }
          range {
            maximum { string_value: "version-2.34" }
            minimum { string_value: "version-0.12" }
          }
        }
      )pb",
      R"pb(
        info { name: "string-measurement" unit: "string-measurement-unit" }
        element {
          measurement_series_id: "NOT_APPLICABLE"
          value { string_value: "version-1.23" }
          valid_values {
            values { string_value: "version-1.23" }
            values { string_value: "version-2.34" }
            values { string_value: "version-0.12" }
          }
        }
      )pb",
      // Boolean measurements.
      R"pb(
        info { name: "boolean-measurement" unit: "boolean-measurement-unit" }
        element {
          measurement_series_id: "NOT_APPLICABLE"
          value { bool_value: false }
          valid_values {
            values { bool_value: false }
            values { bool_value: true }
            values { bool_value: true }
          }
        }
      )pb",
      // List measurements.
      R"pb(
        info {
          name: "list-measurement"
          unit: "list-measurement-unit"
          hardware_info_id: "0"
        }
        element {
          measurement_series_id: "NOT_APPLICABLE"
          value {
            list_value {
              values { number_value: 1.23 }
              values { number_value: 2.34 }
            }
          }
          valid_values {
            values {
              list_value {
                values { number_value: 1.23 }
                values { number_value: 2.34 }
              }
            }
            values {
              list_value {
                values { number_value: 1.23 }
                values { number_value: 2.34 }
              }
            }
            values {
              list_value {
                values { number_value: 1.23 }
                values { number_value: 2.34 }
              }
            }
          }
        }
      )pb",
  };

  const int count = 7;
  ASSERT_EQ(measurements.size(), count);
  ASSERT_EQ(expected_measurements.size(), count);
  for (int i = 0; i < count; i++) {
    EXPECT_THAT(measurements[i],
                Partially(EqualsProto(expected_measurements[i])));
  }
}

}  // namespace
}  // namespace example
}  // namespace meltan
