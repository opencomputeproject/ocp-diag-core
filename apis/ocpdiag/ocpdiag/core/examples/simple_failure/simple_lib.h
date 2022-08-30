// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_EXAMPLES_SIMPLE_FAILURE_SIMPLE_LIB_H_
#define OCPDIAG_CORE_EXAMPLES_SIMPLE_FAILURE_SIMPLE_LIB_H_

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

#endif  // OCPDIAG_CORE_EXAMPLES_SIMPLE_SIMPLE_LIB_H_
