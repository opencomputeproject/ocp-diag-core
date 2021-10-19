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

#ifndef MELTAN_LIB_PARAMS_UTILS_H_
#define MELTAN_LIB_PARAMS_UTILS_H_

#include "grpcpp/impl/codegen/status.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/message.h"
#include "absl/status/status.h"

namespace meltan {
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
}  // namespace meltan

#endif  // MELTAN_LIB_PARAMS_UTILS_H_
