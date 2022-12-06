// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/ocp/proto_converters.h"

#include <variant>

#include "google/protobuf/struct.pb.h"
#include "google/protobuf/util/json_util.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "ocpdiag/core/compat/status_converters.h"
#include "ocpdiag/core/results/ocp/results.pb.h"
#include "ocpdiag/core/results/ocp/structs.h"

namespace ocpdiag::results::internal {

google::protobuf::Value VariantToProto(
    const std::variant<std::string, double, bool>& value) {
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

ocpdiag_results_v2_pb::Validator ValidatorToProto(const Validator& validator) {
  ocpdiag_results_v2_pb::Validator proto;
  proto.set_name(validator.name);
  proto.set_type(
      ocpdiag_results_v2_pb::Validator::ValidatorType(validator.type));
  *proto.mutable_value() = VariantToProto(validator.value);
  return proto;
}

ocpdiag_results_v2_pb::HardwareInfo HardwareInfoToProto(
    const HardwareInfo& info) {
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

ocpdiag_results_v2_pb::SoftwareInfo SoftwareInfoToProto(
    const SoftwareInfo& info) {
  ocpdiag_results_v2_pb::SoftwareInfo proto;
  proto.set_name(info.name);
  proto.set_computer_system(info.computer_system);
  proto.set_version(info.version);
  proto.set_revision(info.revision);
  proto.set_software_type(
      ocpdiag_results_v2_pb::SoftwareInfo::SoftwareType(info.software_type));
  return proto;
}

ocpdiag_results_v2_pb::PlatformInfo PlatformInfoToProto(
    const PlatformInfo& info) {
  ocpdiag_results_v2_pb::PlatformInfo proto;
  proto.set_info(info.info);
  return proto;
}

ocpdiag_results_v2_pb::Subcomponent SubcomponentToProto(
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

ocpdiag_results_v2_pb::MeasurementSeriesStart
MeasurementInfoToMeasurementSeriesStartProto(const MeasurementInfo& info) {
  ocpdiag_results_v2_pb::MeasurementSeriesStart proto;
  proto.set_name(info.name);
  proto.set_unit(info.unit);
  if (info.hardware_info) proto.set_hardware_info_id(info.hardware_info->id());
  if (info.subcomponent)
    *proto.mutable_subcomponent() = SubcomponentToProto(*info.subcomponent);
  for (const Validator& v : info.validators)
    *proto.add_validators() = ValidatorToProto(v);
  return proto;
}

ocpdiag_results_v2_pb::Diagnosis DiagnosisToProto(const Diagnosis& diagnosis) {
  ocpdiag_results_v2_pb::Diagnosis proto;
  proto.set_verdict(diagnosis.verdict);
  proto.set_type(ocpdiag_results_v2_pb::Diagnosis::Type(diagnosis.type));
  proto.set_message(diagnosis.message);
  if (diagnosis.hardware_info)
    proto.set_hardware_info_id(diagnosis.hardware_info->id());
  if (diagnosis.subcomponent)
    *proto.mutable_subcomponent() =
        SubcomponentToProto(*diagnosis.subcomponent);
  return proto;
}

ocpdiag_results_v2_pb::Error ErrorToProto(const Error& error) {
  ocpdiag_results_v2_pb::Error proto;
  proto.set_symptom(error.symptom);
  proto.set_message(error.message);
  for (const RegisteredSoftwareInfo& info : error.software_infos)
    proto.add_software_info_ids(info.id());
  return proto;
}

ocpdiag_results_v2_pb::Measurement MeasurementInfoToMeasurementProto(
    const MeasurementInfo& info) {
  ocpdiag_results_v2_pb::Measurement proto;
  proto.set_name(info.name);
  proto.set_unit(info.unit);
  if (info.hardware_info) proto.set_hardware_info_id(info.hardware_info->id());
  if (info.subcomponent)
    *proto.mutable_subcomponent() =
        SubcomponentToProto(*info.subcomponent);
  for (const Validator& v : info.validators)
    *proto.add_validators() = ValidatorToProto(v);
  return proto;
}

ocpdiag_results_v2_pb::File FileToProto(const File& file) {
  ocpdiag_results_v2_pb::File proto;
  proto.set_display_name(file.display_name);
  proto.set_uri(file.uri);
  proto.set_is_snapshot(file.is_snapshot);
  proto.set_description(file.description);
  proto.set_content_type(file.content_type);
  return proto;
}

google::protobuf::Struct JsonToProtoOrDie(absl::string_view json) {
  google::protobuf::Struct proto;
  CHECK_OK(AsAbslStatus(google::protobuf::util::JsonStringToMessage(json, &proto)))
      << "Must pass a valid JSON string to results objects";
  return proto;
}

}  // namespace ocpdiag::results::internal
