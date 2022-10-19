// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_RESULTS_OCP_VALIDATOR_H_
#define OCPDIAG_CORE_RESULTS_OCP_VALIDATOR_H_

#include "absl/strings/string_view.h"
#include "ocpdiag/core/results/ocp/results.pb.h"
#include "ocpdiag/core/results/ocp/value.h"

namespace ocpdiag::results {

enum class ValidatorType {
  kUnspecified = 0,
  kEqual = 1,
  kNotEqual = 2,
  kLessThan = 3,
  kLessThanOrEqual = 4,
  kGreaterThan = 5,
  kGreaterThanOrEqual = 6,
  kRegexMatch = 7,
  kRegexNoMatch = 8,
  kInSet = 9,
  kNotInSet = 10,
};

struct ValidatorStruct {
  ValidatorStruct(ValidatorType type, const Value& value,
                  absl::string_view name)
      : type(type), value(value), name(name) {}

  const ValidatorType type;
  const Value value;
  std::string name;
};

class Validator {
 public:
  explicit Validator(ValidatorType type, const Value& value,
                     absl::string_view name = "")
      : struct_(type, value, name) {}

  const ValidatorStruct& ToStruct() const { return struct_; }

  //

  ValueType GetValueType() const { return struct_.value.GetType(); }

 private:
  ValidatorStruct struct_;
};

}  // namespace ocpdiag::results

#endif  // OCPDIAG_CORE_RESULTS_OCP_VALIDATOR_H_
