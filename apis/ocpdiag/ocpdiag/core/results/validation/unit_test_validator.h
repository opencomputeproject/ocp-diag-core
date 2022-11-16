// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_RESULTS_UNIT_TEST_VALIDATOR_H_
#define OCPDIAG_CORE_RESULTS_UNIT_TEST_VALIDATOR_H_

#include "absl/status/status.h"
#include "absl/strings/string_view.h"

namespace ocpdiag::results {

// Customizes the options for validating the OCPDiag output.
struct ValidationOptions {
  // Skips any checks relating to the way symptoms are spelled, including
  // invalid characters or words. Symptoms must still be known to surgeon.
  bool skip_regex_checks = false;

  // Skips checking if the symptom is known to surgeon.
  bool skip_surgeon_checks = false;
};

// Validates the OCPDiag output in the record IO file at the given path.
absl::Status ValidateRecordIo(absl::string_view filename,
                              const ValidationOptions &options = {});

}  // namespace ocpdiag::results

#endif  // OCPDIAG_CORE_RESULTS_UNIT_TEST_VALIDATOR_H_
