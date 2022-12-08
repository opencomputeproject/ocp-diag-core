// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/ocp/proto_converters.h"

#include <vector>

#include "gtest/gtest.h"
#include "absl/strings/string_view.h"
#include "ocpdiag/core/results/ocp/structs.h"
#include "ocpdiag/core/testing/proto_matchers.h"

using ::ocpdiag::testing::EqualsProto;

namespace ocpdiag::results::internal {

Subcomponent GetExampleSubcomponent() {
  return {
      .name = "FAN1",
      .type = SubcomponentType::kUnspecified,
      .location = "F0_1",
      .version = "1",
      .revision = "1",
  };
}

MeasurementInfo GetExampleMeasurementInfo() {
  return {
      .name = "measured-fan-speed-100",
      .unit = "RPM",
      .validators = {{
                         .type = ValidatorType::kLessThanOrEqual,
                         .value = 11000.0,
                         .name = "80mm_fan_upper_limit",
                     },
                     {
                         .type = ValidatorType::kGreaterThanOrEqual,
                         .value = 8000.0,
                         .name = "80mm_fan_lower_limit",
                     }},
  };
}

constexpr absl::string_view kMeasurementInfoTextProto = R"pb(
  name: "measured-fan-speed-100"
  unit: "RPM"
  subcomponent {
    name: "FAN1"
    location: "F0_1"
    version: "1"
    revision: "1"
    type: UNSPECIFIED
  }
  validators {
    name: "80mm_fan_upper_limit"
    type: LESS_THAN_OR_EQUAL
    value: { number_value: 11000.0 }
  }
  validators {
    name: "80mm_fan_lower_limit"
    type: GREATER_THAN_OR_EQUAL
    value: { number_value: 8000.0 }
  }
)pb";

TEST(TestHardwareInfoToProto, HardwareInfoStructConvertsSuccessfully) {
  HardwareInfo hardware_info = {
      .name = "primary node",
      .computer_system = "primary_node",
      .location = "MB/DIMM_A1",
      .odata_id = "/redfish/v1/Systems/System.Embedded.1/Memory/DIMMSLOTA1",
      .part_number = "P03052-091",
      .serial_number = "HMA2022029281901",
      .manager = "bmc0",
      .manufacturer = "hynix",
      .manufacturer_part_number = "HMA84GR7AFR4N-VK",
      .part_type = "DIMM",
      .version = "1",
      .revision = "2",
  };
  EXPECT_THAT(
      HardwareInfoToProto(hardware_info), EqualsProto(R"pb(
        name: "primary node"
        computer_system: "primary_node"
        manager: "bmc0"
        location: "MB/DIMM_A1"
        odata_id: "/redfish/v1/Systems/System.Embedded.1/Memory/DIMMSLOTA1"
        part_number: "P03052-091"
        serial_number: "HMA2022029281901"
        manufacturer: "hynix"
        manufacturer_part_number: "HMA84GR7AFR4N-VK"
        part_type: "DIMM"
        version: "1"
        revision: "2"
      )pb"));
}

TEST(TestSoftwareInfoToProto, SoftwareInfoStructConvertsSuccessfully) {
  SoftwareInfo software_info = {
      .name = "bmc_firmware",
      .computer_system = "primary_node",
      .version = "1",
      .revision = "2",
      .software_type = SoftwareType::kFirmware,
  };
  EXPECT_THAT(SoftwareInfoToProto(software_info), EqualsProto(R"pb(
                name: "bmc_firmware"
                computer_system: "primary_node"
                version: "1"
                revision: "2"
                software_type: FIRMWARE
              )pb"));
}

TEST(TestPlatformInfoToProto, PlatformInfoStructConvertsSuccessfully) {
  PlatformInfo platform_info = {
      .info = "memory_optimized",
  };
  EXPECT_THAT(PlatformInfoToProto(platform_info), EqualsProto(R"pb(
                info: "memory_optimized"
              )pb"));
}

TEST(TestMeasurementInfoToMeasurementSeriesStartProto,
     MeasurementInfoStructConvertsToMeasurmentSeriesStartSuccessfully) {
  Subcomponent subcomponent = GetExampleSubcomponent();
  MeasurementInfo measurement_info = GetExampleMeasurementInfo();
  measurement_info.subcomponent = &subcomponent;
  EXPECT_THAT(MeasurementInfoToMeasurementSeriesStartProto(measurement_info),
              EqualsProto(kMeasurementInfoTextProto));
}

//
// TEST(TestMeasurementInfoToMeasurementSeriesStartProto,
//      StringValidatorConvertsSuccessfully) {
//   MeasurementInfo measurement_info = {.name = "string-test",
//                                       .validators = {{
//                                           .type = ValidatorType::kEqual,
//                                           .value = "Test",
//                                       }}};
//   EXPECT_THAT(MeasurementInfoToMeasurementSeriesStartProto(measurement_info),
//               EqualsProto(R"pb(
//                 name: "string-test"
//                 validators {
//                   type: EQUAL
//                   value: { string_value: "Test" }
//                 }
//               )pb"));
// }

TEST(TestMeasurementInfoToMeasurementSeriesStartProto,
     BoolValidatorConvertsSuccessfully) {
  MeasurementInfo measurement_info = {.name = "bool-test",
                                      .validators = {{
                                          .type = ValidatorType::kEqual,
                                          .value = true,
                                      }}};
  EXPECT_THAT(MeasurementInfoToMeasurementSeriesStartProto(measurement_info),
              EqualsProto(R"pb(
                name: "bool-test"
                validators {
                  type: EQUAL
                  value: { bool_value: True }
                }
              )pb"));
}

TEST(TestDiagnosisToProto, DiagnosisStructConvertsSuccessfully) {
  Subcomponent subcomponent = {
      .name = "QPI1",
      .type = SubcomponentType::kBus,
      .location = "CPU-3-2-3",
      .version = "1",
      .revision = "0",
  };
  Diagnosis diagnosis = {
      .verdict = "mlc-intranode-bandwidth-pass",
      .type = DiagnosisType::kPass,
      .message = "intranode bandwidth within threshold.",
      .subcomponent = &subcomponent,
  };
  EXPECT_THAT(DiagnosisToProto(diagnosis), EqualsProto(R"pb(
                verdict: "mlc-intranode-bandwidth-pass"
                type: PASS
                message: "intranode bandwidth within threshold."
                subcomponent {
                  type: BUS
                  name: "QPI1"
                  location: "CPU-3-2-3"
                  version: "1"
                  revision: "0"
                }
              )pb"));
}

TEST(TestErrorToProto, ErrorStructConvertsSuccessfully) {
  Error error = {
      .symptom = "bad-return-code",
      .message = "software exited abnormally.",
  };
  EXPECT_THAT(ErrorToProto(error), EqualsProto(R"pb(
                symptom: "bad-return-code"
                message: "software exited abnormally."
              )pb"));
}

TEST(TestMeasurementInfoToMeasurementProto,
     MeasurementInfoStructConvertsToMeasurmentSuccessfully) {
  Subcomponent subcomponent = GetExampleSubcomponent();
  MeasurementInfo measurement_info = GetExampleMeasurementInfo();
  measurement_info.subcomponent = &subcomponent;
  EXPECT_THAT(MeasurementInfoToMeasurementProto(measurement_info),
              EqualsProto(kMeasurementInfoTextProto));
}

TEST(TestFileToProto, FileStructConvertsSuccessfully) {
  File file = {
      .display_name = "mem_cfg_log",
      .uri = "file:///root/mem_cfg_log",
      .is_snapshot = false,
      .description = "DIMM configuration settings.",
      .content_type = "text/plain",
  };
  EXPECT_THAT(FileToProto(file), EqualsProto(R"pb(
                display_name: "mem_cfg_log"
                uri: "file:///root/mem_cfg_log"
                description: "DIMM configuration settings."
                content_type: "text/plain"
                is_snapshot: false
              )pb"));
}

TEST(TestJsonToProto, ValidJsonConvertsSuccessfully) {
  absl::string_view valid_json = R"json({
    "A field": "with a value",
    "An object": {"Another field": "another value"},
    "A list": ["with", "values"]
  })json";
  EXPECT_THAT(JsonToProtoOrDie(valid_json), EqualsProto(R"pb(
                fields {
                  key: "A field"
                  value { string_value: "with a value" }
                }
                fields {
                  key: "A list"
                  value {
                    list_value {
                      values { string_value: "with" }
                      values { string_value: "values" }
                    }
                  }
                }
                fields {
                  key: "An object"
                  value {
                    struct_value {
                      fields {
                        key: "Another field"
                        value { string_value: "another value" }
                      }
                    }
                  }
                }
              )pb"));
}

TEST(TestJsonToProto, InvalidJsonCausesError) {
  absl::string_view invalid_json = R"json({
    "You forgot a comma": "in this"
    "json": "string"
  })json";
  EXPECT_DEATH(JsonToProtoOrDie(invalid_json), "");
}

}  // namespace ocpdiag::results::internal
