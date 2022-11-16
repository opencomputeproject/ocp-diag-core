// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/validation/unit_test_validator.h"

#include "ocpdiag/core/compat/status_macros.h"
#include "ocpdiag/core/results/recordio_iterator.h"
#include "ocpdiag/core/results/results.pb.h"

namespace ocpdiag::results {
namespace {
namespace rpb = ::ocpdiag::results_pb;

absl::Status Validate(const rpb::OutputArtifact &artifact,
                      const ValidationOptions &options) {
  return absl::OkStatus();
}

}  // namespace

absl::Status ValidateRecordIo(absl::string_view filename,
                              const ValidationOptions &options) {
  RecordIoContainer<rpb::OutputArtifact> reader(filename);
  for (const rpb::OutputArtifact &artifact : reader) {
    RETURN_IF_ERROR(Validate(artifact, options));
  }
  return absl::OkStatus();
}

}  // namespace ocpdiag::results
