// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_RESULTS_OCP_PROTO_CONVERTERS_H_
#define OCPDIAG_CORE_RESULTS_OCP_PROTO_CONVERTERS_H_

#include "google/protobuf/struct.pb.h"
#include "absl/strings/string_view.h"
#include "ocpdiag/core/results/ocp/results.pb.h"
#include "ocpdiag/core/results/ocp/structs.h"

namespace ocpdiag::results::internal {

// Converts the HardwareInfo struct to its corresponding protobuf
ocpdiag_results_v2_pb::HardwareInfo HardwareInfoToProto(
    const HardwareInfo& info);

// Converts the SoftwareInfo struct to its corresponding protobuf
ocpdiag_results_v2_pb::SoftwareInfo SoftwareInfoToProto(
    const SoftwareInfo& info);

// Converts the PlatformInfo struct to its corresponding protobuf
ocpdiag_results_v2_pb::PlatformInfo PlatformInfoToProto(
    const PlatformInfo& info);

// Converts the MeasurementInfo struct into the MeasurementSeriesStart proto
ocpdiag_results_v2_pb::MeasurementSeriesStart
MeasurementInfoToMeasurementSeriesStartProto(const MeasurementInfo& info);

// Converts the Diagnosis struct to its corresponding protobuf
ocpdiag_results_v2_pb::Diagnosis DiagnosisToProto(const Diagnosis& diagnosis);

// Converts the Error struct to its corresponding protobuf
ocpdiag_results_v2_pb::Error ErrorToProto(const Error& error);

// Converts the MeasurementInfo struct into the Measurement proto
ocpdiag_results_v2_pb::Measurement MeasurementInfoToMeasurementProto(
    const MeasurementInfo& info);

// Converts the File struct to its corresponding protobuf
ocpdiag_results_v2_pb::File FileToProto(const File& file);

// Converts a JSON string to a generic protobuf struct or throws a fatal CHECK
// error
google::protobuf::Struct JsonToProtoOrDie(absl::string_view json);

}  // namespace ocpdiag::results::internal

#endif  // OCPDIAG_CORE_RESULTS_OCP_PROTO_CONVERTERS_H_
