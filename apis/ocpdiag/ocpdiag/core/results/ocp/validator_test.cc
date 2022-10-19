// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/ocp/validator.h"

#include "gtest/gtest.h"
#include "ocpdiag/core/results/ocp/value.h"

namespace ocpdiag::results {

TEST(TestValidator, CreateValidator) {
  Validator validator(ValidatorType::kEqual, Value::FromString("test"));
  EXPECT_EQ(ValueType::kString, validator.GetValueType());
}

}  // namespace ocpdiag::results
