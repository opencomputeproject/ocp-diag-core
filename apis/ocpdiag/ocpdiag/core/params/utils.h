// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_PARAMS_UTILS_H_
#define OCPDIAG_CORE_PARAMS_UTILS_H_

#include "google/protobuf/message.h"
#include "absl/status/status.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace ocpdiag {
namespace params {

// Implement missing json_utils.h function to take an input JSON stream and
// merge it into an existing message type.
// Unlike the normal conversions, this is safe for an empty input.
absl::Status MergeFromJsonStream(google::protobuf::io::ZeroCopyInputStream* json_stream,
                                 google::protobuf::Message* output);

// Parses a JSON file containing a proto message.
absl::Status JsonFileToMessage(const char* file_path, google::protobuf::Message* output);

// Get JSON-formatted parameters from standard input and fill it in params
// proto.
absl::Status GetParams(google::protobuf::Message* params);

// Return a version string.
absl::string_view GetVersion();

}  // namespace params
}  // namespace ocpdiag

#endif  // OCPDIAG_CORE_PARAMS_UTILS_H_
