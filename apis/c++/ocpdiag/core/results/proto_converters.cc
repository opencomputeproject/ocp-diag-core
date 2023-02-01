// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/proto_converters.h"

#include "google/protobuf/struct.pb.h"
#include "google/protobuf/util/json_util.h"
#include "google/protobuf/util/time_util.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/string_view.h"
#include "ocpdiag/core/compat/status_converters.h"
#include "ocpdiag/core/results/dut_info.h"
#include "ocpdiag/core/results/results.pb.h"
#include "ocpdiag/core/results/structs.h"
#include "ocpdiag/core/results/variant.h"

namespace ocpdiag::results::internal {

//
google::protobuf::Value VariantToProto(const Variant& value) {
  google::protobuf::Value proto;
  if (auto* str_val = std::get_if<std::string>(&value); str_val != nullptr) {
    proto.set_string_value(*str_val);
  } else if (auto* bool_val = std::get_if<bool>(&value); bool_val != nullptr) {
    proto.set_bool_value(*bool_val);
  } else if (auto* double_val = std::get_if<double>(&value);
             double_val != nullptr) {
    proto.set_number_value(*double_val);
  } else {
    LOG(FATAL) << "Tried to convert an invalid value.";
  }
  return proto;
}

ocpdiag_results_v2_pb::Validator StructToProto(const Validator& validator) {
  ocpdiag_results_v2_pb::Validator proto;
  proto.set_name(validator.name);
  proto.set_type(
      ocpdiag_results_v2_pb::Validator::ValidatorType(validator.type));

  if (validator.value.size() == 1) {
    *proto.mutable_value() = VariantToProto(validator.value[0]);
  } else {
    for (const Variant& value : validator.value) {
      *proto.mutable_value()->mutable_list_value()->add_values() =
          VariantToProto(value);
    }
  }
  return proto;
}

ocpdiag_results_v2_pb::HardwareInfo StructToProto(const HardwareInfo& info) {
  ocpdiag_results_v2_pb::HardwareInfo proto;
  proto.set_name(info.name);
  proto.set_computer_system(info.computer_system);
  proto.set_location(info.location);
  proto.set_odata_id(info.odata_id);
  proto.set_part_number(info.part_number);
  proto.set_serial_number(info.serial_number);
  proto.set_manager(info.manager);
  proto.set_manufacturer(info.manufacturer);
  proto.set_manufacturer_part_number(info.manufacturer_part_number);
  proto.set_part_type(info.part_type);
  proto.set_version(info.version);
  proto.set_revision(info.revision);
  return proto;
}

ocpdiag_results_v2_pb::SoftwareInfo StructToProto(const SoftwareInfo& info) {
  ocpdiag_results_v2_pb::SoftwareInfo proto;
  proto.set_name(info.name);
  proto.set_computer_system(info.computer_system);
  proto.set_version(info.version);
  proto.set_revision(info.revision);
  proto.set_software_type(
      ocpdiag_results_v2_pb::SoftwareInfo::SoftwareType(info.software_type));
  return proto;
}

ocpdiag_results_v2_pb::PlatformInfo StructToProto(const PlatformInfo& info) {
  ocpdiag_results_v2_pb::PlatformInfo proto;
  proto.set_info(info.info);
  return proto;
}

ocpdiag_results_v2_pb::Subcomponent StructToProto(
    const Subcomponent& subcomponent) {
  ocpdiag_results_v2_pb::Subcomponent proto;
  proto.set_name(subcomponent.name);
  proto.set_type(
      ocpdiag_results_v2_pb::Subcomponent::SubcomponentType(subcomponent.type));
  proto.set_location(subcomponent.location);
  proto.set_version(subcomponent.version);
  proto.set_revision(subcomponent.revision);
  return proto;
}

ocpdiag_results_v2_pb::MeasurementSeriesStart StructToProto(
    const MeasurementSeriesStart& measurement_series_start) {
  ocpdiag_results_v2_pb::MeasurementSeriesStart proto;
  proto.set_name(measurement_series_start.name);
  proto.set_unit(measurement_series_start.unit);
  if (measurement_series_start.hardware_info.has_value())
    proto.set_hardware_info_id(measurement_series_start.hardware_info->id());
  if (measurement_series_start.subcomponent.has_value())
    *proto.mutable_subcomponent() =
        StructToProto(*measurement_series_start.subcomponent);
  for (const Validator& v : measurement_series_start.validators)
    *proto.add_validators() = StructToProto(v);
  *proto.mutable_metadata() =
      JsonToProtoOrDie(measurement_series_start.metadata_json);
  return proto;
}

ocpdiag_results_v2_pb::MeasurementSeriesElement StructToProto(
    const MeasurementSeriesElement& measurement_series_element) {
  ocpdiag_results_v2_pb::MeasurementSeriesElement proto;
  *proto.mutable_value() = VariantToProto(measurement_series_element.value);
  if (measurement_series_element.timestamp.has_value()) {
    *proto.mutable_timestamp() = google::protobuf::util::TimeUtil::TimevalToTimestamp(
        *measurement_series_element.timestamp);
  }
  *proto.mutable_metadata() =
      JsonToProtoOrDie(measurement_series_element.metadata_json);
  return proto;
}

ocpdiag_results_v2_pb::Measurement StructToProto(
    const Measurement& measurement) {
  ocpdiag_results_v2_pb::Measurement proto;
  *proto.mutable_value() = VariantToProto(measurement.value);
  proto.set_name(measurement.name);
  proto.set_unit(measurement.unit);
  if (measurement.hardware_info.has_value())
    proto.set_hardware_info_id(measurement.hardware_info->id());
  if (measurement.subcomponent.has_value())
    *proto.mutable_subcomponent() = StructToProto(*measurement.subcomponent);
  for (const Validator& v : measurement.validators)
    *proto.add_validators() = StructToProto(v);
  *proto.mutable_metadata() = JsonToProtoOrDie(measurement.metadata_json);
  return proto;
}

ocpdiag_results_v2_pb::Diagnosis StructToProto(const Diagnosis& diagnosis) {
  ocpdiag_results_v2_pb::Diagnosis proto;
  proto.set_verdict(diagnosis.verdict);
  proto.set_type(ocpdiag_results_v2_pb::Diagnosis::Type(diagnosis.type));
  proto.set_message(diagnosis.message);
  if (diagnosis.hardware_info.has_value())
    proto.set_hardware_info_id(diagnosis.hardware_info->id());
  if (diagnosis.subcomponent.has_value())
    *proto.mutable_subcomponent() = StructToProto(*diagnosis.subcomponent);
  return proto;
}

ocpdiag_results_v2_pb::Error StructToProto(const Error& error) {
  ocpdiag_results_v2_pb::Error proto;
  proto.set_symptom(error.symptom);
  proto.set_message(error.message);
  for (const RegisteredSoftwareInfo& info : error.software_infos)
    proto.add_software_info_ids(info.id());
  return proto;
}

ocpdiag_results_v2_pb::File StructToProto(const File& file) {
  ocpdiag_results_v2_pb::File proto;
  proto.set_display_name(file.display_name);
  proto.set_uri(file.uri);
  proto.set_is_snapshot(file.is_snapshot);
  proto.set_description(file.description);
  proto.set_content_type(file.content_type);
  return proto;
}

ocpdiag_results_v2_pb::TestRunStart StructToProto(
    const TestRunStart& test_run_start) {
  ocpdiag_results_v2_pb::TestRunStart proto;
  proto.set_name(test_run_start.name);
  proto.set_version(test_run_start.version);
  proto.set_command_line(test_run_start.command_line);
  *proto.mutable_parameters() =
      JsonToProtoOrDie(test_run_start.parameters_json);
  *proto.mutable_metadata() = JsonToProtoOrDie(test_run_start.metadata_json);
  return proto;
}

ocpdiag_results_v2_pb::Log StructToProto(const Log& log) {
  ocpdiag_results_v2_pb::Log proto;
  proto.set_message(log.message);
  proto.set_severity(ocpdiag_results_v2_pb::Log::Severity(log.severity));
  return proto;
}

ocpdiag_results_v2_pb::Extension StructToProto(const Extension& extension) {
  ocpdiag_results_v2_pb::Extension proto;
  proto.set_name(extension.name);
  *proto.mutable_content() = JsonToProtoOrDie(extension.content_json);
  return proto;
}

google::protobuf::Struct JsonToProtoOrDie(absl::string_view json) {
  google::protobuf::Struct proto;
  if (json.empty()) return proto;
  absl::Status status =
      AsAbslStatus(google::protobuf::util::JsonStringToMessage(json, &proto));
  CHECK_OK(status) << "Must pass a valid JSON string to results objects: "
                   << status.ToString();
  return proto;
}

ocpdiag_results_v2_pb::DutInfo DutInfoToProto(const DutInfo& dut_info) {
  ocpdiag_results_v2_pb::DutInfo proto;
  proto.set_dut_info_id(dut_info.id());
  proto.set_name(dut_info.name());
  *proto.mutable_metadata() = JsonToProtoOrDie(dut_info.GetMetadataJson());
  for (const PlatformInfo& platform_info : dut_info.GetPlatformInfos())
    *proto.add_platform_infos() = StructToProto(platform_info);
  for (const HardwareInfo& hardware_info : dut_info.GetHardwareInfos())
    *proto.add_hardware_infos() = StructToProto(hardware_info);
  for (const SoftwareInfo& software_info : dut_info.GetSoftwareInfos())
    *proto.add_software_infos() = StructToProto(software_info);
  return proto;
}

Variant ProtoToVariant(google::protobuf::Value value) {
  if (value.has_string_value()) {
    return Variant(value.string_value());
  } else if (value.has_number_value()) {
    return value.number_value();
  } else if (value.has_bool_value()) {
    return value.bool_value();
  }
  LOG(FATAL) << "Tried to convert an invalid value protobuf to a Variant.";
}

SchemaVersionOutput ProtoToStruct(
    const ocpdiag_results_v2_pb::SchemaVersion& schema_version) {
  return {.major = schema_version.major(), .minor = schema_version.minor()};
}

PlatformInfoOutput ProtoToStruct(
    const ocpdiag_results_v2_pb::PlatformInfo& platform_info) {
  return {.info = platform_info.info()};
}

HardwareInfoOutput ProtoToStruct(
    const ocpdiag_results_v2_pb::HardwareInfo& hardware_info) {
  return {
      .hardware_info_id = hardware_info.hardware_info_id(),
      .name = hardware_info.name(),
      .computer_system = hardware_info.computer_system(),
      .location = hardware_info.location(),
      .odata_id = hardware_info.odata_id(),
      .part_number = hardware_info.part_number(),
      .serial_number = hardware_info.serial_number(),
      .manager = hardware_info.manager(),
      .manufacturer = hardware_info.manufacturer(),
      .manufacturer_part_number = hardware_info.manufacturer_part_number(),
      .part_type = hardware_info.part_type(),
      .version = hardware_info.version(),
      .revision = hardware_info.revision(),
  };
}

SoftwareInfoOutput ProtoToStruct(
    const ocpdiag_results_v2_pb::SoftwareInfo& software_info) {
  return {
      .software_info_id = software_info.software_info_id(),
      .name = software_info.name(),
      .computer_system = software_info.computer_system(),
      .version = software_info.version(),
      .revision = software_info.revision(),
      .software_type = SoftwareType(software_info.software_type()),
  };
}

DutInfoOutput ProtoToStruct(const ocpdiag_results_v2_pb::DutInfo& dut_info) {
  std::vector<PlatformInfoOutput> platform_infos;
  for (const ocpdiag_results_v2_pb::PlatformInfo& platform_info :
       dut_info.platform_infos())
    platform_infos.push_back(ProtoToStruct(platform_info));
  std::vector<HardwareInfoOutput> hardware_infos;
  for (const ocpdiag_results_v2_pb::HardwareInfo& hardware_info :
       dut_info.hardware_infos())
    hardware_infos.push_back(ProtoToStruct(hardware_info));
  std::vector<SoftwareInfoOutput> software_infos;
  for (const ocpdiag_results_v2_pb::SoftwareInfo& software_info :
       dut_info.software_infos())
    software_infos.push_back(ProtoToStruct(software_info));
  return {
      .dut_info_id = dut_info.dut_info_id(),
      .name = dut_info.name(),
      .metadata_json = ProtoToJsonOrDie(dut_info.metadata()),
      .platform_infos = platform_infos,
      .hardware_infos = hardware_infos,
      .software_infos = software_infos,
  };
}

TestRunStartOutput ProtoToStruct(
    const ocpdiag_results_v2_pb::TestRunStart& test_run_start) {
  return {
      .name = test_run_start.name(),
      .version = test_run_start.version(),
      .command_line = test_run_start.command_line(),
      .parameters_json = ProtoToJsonOrDie(test_run_start.parameters()),
      .dut_info = ProtoToStruct(test_run_start.dut_info()),
      .metadata_json = ProtoToJsonOrDie(test_run_start.metadata()),
  };
}

TestRunEndOutput ProtoToStruct(
    const ocpdiag_results_v2_pb::TestRunEnd& test_run_end) {
  return {
      .status = TestStatus(test_run_end.status()),
      .result = TestResult(test_run_end.result()),
  };
}

LogOutput ProtoToStruct(const ocpdiag_results_v2_pb::Log& log) {
  return {.severity = LogSeverity(log.severity()), .message = log.message()};
}

ErrorOutput ProtoToStruct(const ocpdiag_results_v2_pb::Error& error) {
  return {
      .symptom = error.symptom(),
      .message = error.message(),
      .software_info_ids = std::vector<std::string>(
          error.software_info_ids().begin(), error.software_info_ids().end()),
  };
}

TestStepStartOutput ProtoToStruct(
    const ocpdiag_results_v2_pb::TestStepStart& test_step_start) {
  return {.name = test_step_start.name()};
}

TestStepEndOutput ProtoToStruct(
    const ocpdiag_results_v2_pb::TestStepEnd& test_step_end) {
  return {.status = TestStatus(test_step_end.status())};
}

SubcomponentOutput ProtoToStruct(
    const ocpdiag_results_v2_pb::Subcomponent subcomponent) {
  return {
      .name = subcomponent.name(),
      .type = SubcomponentType(subcomponent.type()),
      .location = subcomponent.location(),
      .version = subcomponent.version(),
      .revision = subcomponent.revision(),
  };
}

ValidatorOutput ProtoToStruct(
    const ocpdiag_results_v2_pb::Validator& validator) {
  std::vector<Variant> values;
  if (validator.value().has_list_value()) {
    for (const google::protobuf::Value& value :
         validator.value().list_value().values())
      values.push_back(ProtoToVariant(value));
  } else {
    values.push_back(ProtoToVariant(validator.value()));
  }
  return {
      .type = ValidatorType(validator.type()),
      .value = values,
      .name = validator.name(),
  };
}

MeasurementOutput ProtoToStruct(
    const ocpdiag_results_v2_pb::Measurement& measurement) {
  std::optional<SubcomponentOutput> subcomponent = {};
  if (measurement.has_subcomponent())
    subcomponent = ProtoToStruct(measurement.subcomponent());
  std::vector<ValidatorOutput> validators;
  for (const ocpdiag_results_v2_pb::Validator& validator :
       measurement.validators())
    validators.push_back(ProtoToStruct(validator));
  return {
      .name = measurement.name(),
      .unit = measurement.unit(),
      .hardware_info_id = measurement.hardware_info_id(),
      .subcomponent = subcomponent,
      .validators = validators,
      .value = ProtoToVariant(measurement.value()),
      .metadata_json = ProtoToJsonOrDie(measurement.metadata()),
  };
}

MeasurementSeriesStartOutput ProtoToStruct(
    const ocpdiag_results_v2_pb::MeasurementSeriesStart&
        measurement_series_start) {
  std::optional<SubcomponentOutput> subcomponent = {};
  if (measurement_series_start.has_subcomponent())
    subcomponent = ProtoToStruct(measurement_series_start.subcomponent());
  std::vector<ValidatorOutput> validators;
  for (const ocpdiag_results_v2_pb::Validator& validator :
       measurement_series_start.validators())
    validators.push_back(ProtoToStruct(validator));
  return {
      .measurement_series_id = measurement_series_start.measurement_series_id(),
      .name = measurement_series_start.name(),
      .unit = measurement_series_start.unit(),
      .hardware_info_id = measurement_series_start.hardware_info_id(),
      .subcomponent = subcomponent,
      .validators = validators,
      .metadata_json = ProtoToJsonOrDie(measurement_series_start.metadata()),
  };
}

MeasurementSeriesElementOutput ProtoToStruct(
    const ocpdiag_results_v2_pb::MeasurementSeriesElement& element) {
  return {
      .index = element.index(),
      .measurement_series_id = element.measurement_series_id(),
      .value = ProtoToVariant(element.value()),
      .timestamp =
          google::protobuf::util::TimeUtil::TimestampToTimeval(element.timestamp()),
      .metadata_json = ProtoToJsonOrDie(element.metadata()),
  };
}

MeasurementSeriesEndOutput ProtoToStruct(
    const ocpdiag_results_v2_pb::MeasurementSeriesEnd& measurement_series_end) {
  return {
      .measurement_series_id = measurement_series_end.measurement_series_id(),
      .total_count = measurement_series_end.total_count(),
  };
}

DiagnosisOutput ProtoToStruct(
    const ocpdiag_results_v2_pb::Diagnosis& diagnosis) {
  std::optional<SubcomponentOutput> subcomponent = {};
  if (diagnosis.has_subcomponent())
    subcomponent = ProtoToStruct(diagnosis.subcomponent());
  return {
      .verdict = diagnosis.verdict(),
      .type = DiagnosisType(diagnosis.type()),
      .message = diagnosis.message(),
      .hardware_info_id = diagnosis.hardware_info_id(),
      .subcomponent = subcomponent,
  };
}

FileOutput ProtoToStruct(const ocpdiag_results_v2_pb::File& file) {
  return {
      .display_name = file.display_name(),
      .uri = file.uri(),
      .is_snapshot = file.is_snapshot(),
      .description = file.description(),
      .content_type = file.content_type(),
  };
}

ExtensionOutput ProtoToStruct(
    const ocpdiag_results_v2_pb::Extension& extension) {
  return {
      .name = extension.name(),
      .content_json = ProtoToJsonOrDie(extension.content()),
  };
}

TestStepArtifact ProtoToStruct(
    const ocpdiag_results_v2_pb::TestStepArtifact& artifact) {
  TestStepVariant variant;
  switch (artifact.artifact_case()) {
    case ocpdiag_results_v2_pb::TestStepArtifact::ArtifactCase::kTestStepStart:
      variant = ProtoToStruct(artifact.test_step_start());
      break;
    case ocpdiag_results_v2_pb::TestStepArtifact::ArtifactCase::kTestStepEnd:
      variant = ProtoToStruct(artifact.test_step_end());
      break;
    case ocpdiag_results_v2_pb::TestStepArtifact::ArtifactCase::kMeasurement:
      variant = ProtoToStruct(artifact.measurement());
      break;
    case ocpdiag_results_v2_pb::TestStepArtifact::ArtifactCase::
        kMeasurementSeriesStart:
      variant = ProtoToStruct(artifact.measurement_series_start());
      break;
    case ocpdiag_results_v2_pb::TestStepArtifact::ArtifactCase::
        kMeasurementSeriesEnd:
      variant = ProtoToStruct(artifact.measurement_series_end());
      break;
    case ocpdiag_results_v2_pb::TestStepArtifact::ArtifactCase::
        kMeasurementSeriesElement:
      variant = ProtoToStruct(artifact.measurement_series_element());
      break;
    case ocpdiag_results_v2_pb::TestStepArtifact::ArtifactCase::kDiagnosis:
      variant = ProtoToStruct(artifact.diagnosis());
      break;
    case ocpdiag_results_v2_pb::TestStepArtifact::ArtifactCase::kError:
      variant = ProtoToStruct(artifact.error());
      break;
    case ocpdiag_results_v2_pb::TestStepArtifact::ArtifactCase::kFile:
      variant = ProtoToStruct(artifact.file());
      break;
    case ocpdiag_results_v2_pb::TestStepArtifact::ArtifactCase::kLog:
      variant = ProtoToStruct(artifact.log());
      break;
    case ocpdiag_results_v2_pb::TestStepArtifact::ArtifactCase::kExtension:
      variant = ProtoToStruct(artifact.extension());
      break;
    default:
      LOG(FATAL) << "Tried to convert an empty or unexepected TestStepArtifact "
                    "from proto";
  }
  return {
      .artifact = variant,
      .test_step_id = artifact.test_step_id(),
  };
}

TestRunArtifact ProtoToStruct(
    const ocpdiag_results_v2_pb::TestRunArtifact& artifact) {
  switch (artifact.artifact_case()) {
    case ocpdiag_results_v2_pb::TestRunArtifact::ArtifactCase::kTestRunStart:
      return {.artifact = ProtoToStruct(artifact.test_run_start())};
    case ocpdiag_results_v2_pb::TestRunArtifact::ArtifactCase::kTestRunEnd:
      return {.artifact = ProtoToStruct(artifact.test_run_end())};
    case ocpdiag_results_v2_pb::TestRunArtifact::ArtifactCase::kLog:
      return {.artifact = ProtoToStruct(artifact.log())};
    case ocpdiag_results_v2_pb::TestRunArtifact::ArtifactCase::kError:
      return {.artifact = ProtoToStruct(artifact.error())};
    default:
      LOG(FATAL) << "Tried to convert an empty or unexepected TestRunArtifact "
                    "from proto";
  }
}

OutputArtifact ProtoToStruct(
    const ocpdiag_results_v2_pb::OutputArtifact& artifact) {
  OutputVariant variant;
  switch (artifact.artifact_case()) {
    case ocpdiag_results_v2_pb::OutputArtifact::ArtifactCase::kSchemaVersion:
      variant = ProtoToStruct(artifact.schema_version());
      break;
    case ocpdiag_results_v2_pb::OutputArtifact::ArtifactCase::kTestRunArtifact:
      variant = ProtoToStruct(artifact.test_run_artifact());
      break;
    case ocpdiag_results_v2_pb::OutputArtifact::ArtifactCase::kTestStepArtifact:
      variant = ProtoToStruct(artifact.test_step_artifact());
      break;
    default:
      LOG(FATAL) << "Tried to convert an empty or unexepected OutputArtifact "
                    "from proto";
  }
  return {
      .artifact = variant,
      .sequence_number = artifact.sequence_number(),
      .timestamp =
          google::protobuf::util::TimeUtil::TimestampToTimeval(artifact.timestamp()),
  };
}

std::string ProtoToJsonOrDie(const google::protobuf::Struct& proto) {
  std::string json;
  absl::Status status =
      AsAbslStatus(google::protobuf::util::MessageToJsonString(proto, &json));
  CHECK_OK(status) << "Issue converting struct type to JSON";
  return json;
}

}  // namespace ocpdiag::results::internal
