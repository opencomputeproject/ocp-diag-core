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

#ifndef MELTAN_CORE_EXAMPLES_SIMPLE_FAILURE_SIMPLE_LIB_H_
#define MELTAN_CORE_EXAMPLES_SIMPLE_FAILURE_SIMPLE_LIB_H_

#include "google/protobuf/struct.pb.h"
#include "absl/types/span.h"
#include "ocpdiag/core/params/utils.h"
#include "ocpdiag/core/results/results.h"

namespace ocpdiag {
namespace example {

// Adds a measurement with range to step.
void AddMeasurementWithRange(ocpdiag::results::TestStep* step, std::string name,
                             google::protobuf::Value val,
                             google::protobuf::Value max,
                             google::protobuf::Value min,
                             const ocpdiag::results::HwRecord* hwRecord);

// Adds a measurement with valid values to step.
void AddMeasurementWithValues(
    ocpdiag::results::TestStep* step, std::string name,
    google::protobuf::Value val,
    absl::Span<const google::protobuf::Value> valid_vals,
    const ocpdiag::results::HwRecord* hwRecord);

// Adds all types of measurements.
void AddAllMeasurementTypes(ocpdiag::results::TestStep* step,
                            const ocpdiag::results::HwRecord* hwRecord);

}  // namespace example
}  // namespace ocpdiag

#endif  // MELTAN_CORE_EXAMPLES_SIMPLE_SIMPLE_LIB_H_
