// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_RESULTS_OCP_VALUE_H_
#define OCPDIAG_CORE_RESULTS_OCP_VALUE_H_

#include <string>

namespace ocpdiag::results {

enum class ValueType { kUnknown = 0, kString = 1, kNumber = 2, kBool = 3 };

struct ValueStruct {
  std::string string_value;
  double number_value;
  bool bool_value;
  ValueType type;
};

class Value {
 public:
  Value(ValueStruct value_struct) : struct_(value_struct) {}

  static Value FromString(std::string string_value) {
    ValueStruct value_struct;
    value_struct.string_value = string_value;
    value_struct.type = ValueType::kString;
    return Value(value_struct);
  }

  static Value FromNumber(double number_value) {
    ValueStruct value_struct;
    value_struct.number_value = number_value;
    value_struct.type = ValueType::kNumber;
    return Value(value_struct);
  }

  static Value FromBool(bool bool_value) {
    ValueStruct value_struct;
    value_struct.bool_value = bool_value;
    value_struct.type = ValueType::kBool;
    return Value(value_struct);
  }

  const ValueStruct& ToStruct() const { return struct_; }

  const ValueType GetType() const { return struct_.type; }

 private:
  ValueStruct struct_;
};

}  // namespace ocpdiag::results

#endif  // OCPDIAG_CORE_RESULTS_OCP_VALUE_H_
