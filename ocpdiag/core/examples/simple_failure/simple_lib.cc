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

#include "ocpdiag/core/examples/simple_failure/simple_lib.h"

namespace ocpdiag {
namespace example {

using ::google::protobuf::Value;
using ::ocpdiag::results::HwRecord;
using ::ocpdiag::results::TestStep;
using ::ocpdiag::results_pb::MeasurementElement;
using ::ocpdiag::results_pb::MeasurementInfo;

constexpr int32_t kProtoValueTypeCnt = 5;

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

  // Null value.
  names[0] = "null-measurement";
  vals[0].set_null_value(google::protobuf::NULL_VALUE);
  maximums[0].set_null_value(google::protobuf::NULL_VALUE);
  minimums[0].set_null_value(google::protobuf::NULL_VALUE);

  // Number value.
  names[1] = "number-measurement";
  vals[1].set_number_value(1.23);
  maximums[1].set_number_value(2.34);
  minimums[1].set_number_value(0.12);

  // String value.
  names[2] = "string-measurement";
  vals[2].set_string_value("version-1.23");
  maximums[2].set_string_value("version-2.34");
  minimums[2].set_string_value("version-0.12");

  // Boolean value.
  names[3] = "boolean-measurement";
  vals[3].set_bool_value(false);
  maximums[3].set_bool_value(true);
  minimums[3].set_bool_value(true);

  // List Value.
  names[4] = "list-measurement";
  google::protobuf::ListValue* list_val = vals[4].mutable_list_value();
  Value val_key1(vals[1]), val_key2(maximums[1]);
  *list_val->add_values() = val_key1;
  *list_val->add_values() = val_key2;
  *maximums[4].mutable_list_value() = *list_val;
  *minimums[4].mutable_list_value() = *list_val;

  // Send measurements.
  for (int i = 0; i < kProtoValueTypeCnt; i++) {
    // Range for lists, null, or bool doesn't make sense.
    if (names[i] != "list-measurement" && names[i] != "null-measurement" &&
        names[i] != "boolean-measurement") {
      AddMeasurementWithRange(step, names[i], vals[i], maximums[i], minimums[i],
                              hwRecord);
    }
    AddMeasurementWithValidValues(
        step, names[i], vals[i],
        absl::Span<const Value>{vals[i], maximums[i], minimums[i]}, hwRecord);
  }
}

}  // namespace example
}  // namespace ocpdiag
