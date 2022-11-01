// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/examples/simple/simple_lib.h"

namespace ocpdiag {
namespace example {

using ::google::protobuf::Value;
using ::ocpdiag::results::HwRecord;
using ::ocpdiag::results::TestStep;
using ::ocpdiag::results_pb::MeasurementElement;
using ::ocpdiag::results_pb::MeasurementInfo;

constexpr int32_t kProtoValueTypeCnt = 2;

void AddMeasurementWithRange(TestStep* step, std::string name, Value val,
                             Value max, Value min, const HwRecord* hwRecord) {
  MeasurementInfo info;
  info.set_unit(absl::StrCat(name, "-unit"));
  info.set_name(std::move(name));
  MeasurementElement element;
  ocpdiag::results_pb::MeasurementElement::Range range;
  *range.mutable_maximum() = std::move(max);
  *range.mutable_minimum() = std::move(min);
  *element.mutable_range() = std::move(range);
  *element.mutable_value() = std::move(val);
  step->AddMeasurement(info, element, hwRecord);
}

void AddMeasurementWithValidValues(TestStep* step, std::string name, Value val,
                                   absl::Span<const Value> valid_vals,
                                   const HwRecord* hwRecord) {
  MeasurementInfo info;
  info.set_unit(absl::StrCat(name, "-unit"));
  info.set_name(std::move(name));
  MeasurementElement ele;
  *ele.mutable_value() = std::move(val);
  for (const Value& v : valid_vals) {
    *ele.mutable_valid_values()->add_values() = v;
  }
  step->AddMeasurement(info, ele, hwRecord);
}

void AddAllMeasurementTypes(TestStep* step, const HwRecord* hwRecord) {
  std::vector<std::string> names(kProtoValueTypeCnt);
  std::vector<Value> vals(kProtoValueTypeCnt), maximums(kProtoValueTypeCnt),
      minimums(kProtoValueTypeCnt);

  // Number value.
  names[0] = "number-measurement";
  vals[0].set_number_value(1.23);
  maximums[0].set_number_value(2.34);
  minimums[0].set_number_value(0.12);

  // String value.
  names[1] = "string-measurement";
  vals[1].set_string_value("version-1.23");
  maximums[1].set_string_value("version-2.34");
  minimums[1].set_string_value("version-0.12");

  // Send measurements.
  for (int i = 0; i < kProtoValueTypeCnt; i++) {
    AddMeasurementWithRange(step, names[i], vals[i], maximums[i], minimums[i],
                            hwRecord);
    AddMeasurementWithValidValues(
        step, names[i], vals[i],
        absl::Span<const Value>{vals[i], maximums[i], minimums[i]}, hwRecord);
  }
}

}  // namespace example
}  // namespace ocpdiag
