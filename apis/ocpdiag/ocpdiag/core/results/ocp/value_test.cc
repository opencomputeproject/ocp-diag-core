// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/ocp/value.h"

#include "gtest/gtest.h"

namespace ocpdiag::results {

TEST(TestValue, CreateValueFromString) {
  std::string str_value = "my_str";
  ValueStruct val = Value::FromString(str_value).ToStruct();
  EXPECT_EQ(val.string_value, str_value);
  EXPECT_EQ(val.type, ValueType::kString);
}

TEST(TestValue, CreateValueFromNumber) {
  double num_value = 3.4;
  ValueStruct val = Value::FromNumber(num_value).ToStruct();
  EXPECT_EQ(val.number_value, 3.4);
  EXPECT_EQ(val.type, ValueType::kNumber);
}

TEST(TestValue, CreateValueFromBool) {
  bool bool_value = true;
  ValueStruct val = Value::FromBool(bool_value).ToStruct();
  EXPECT_EQ(val.bool_value, true);
  EXPECT_EQ(val.type, ValueType::kBool);
}

}  // namespace ocpdiag::results
