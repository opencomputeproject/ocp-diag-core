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

#include "ocpdiag/core/params/utils.h"

#include <fcntl.h>
#include <unistd.h>

#include <memory>
#include <string>

#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/util/json_util.h"
#include "google/protobuf/util/type_resolver.h"
#include "google/protobuf/util/type_resolver_util.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "ocpdiag/core/compat/status_converters.h"
#include "ocpdiag/core/params/utils_version.h"

namespace ocpdiag {
namespace params {

absl::Status MergeFromJsonStream(google::protobuf::io::ZeroCopyInputStream* json_stream,
                                 google::protobuf::Message* output) {
  const std::string type_prefix = "type.googleapis.com";
  const std::string type_url =
      absl::StrCat(type_prefix, "/", output->GetDescriptor()->full_name());
  std::unique_ptr<google::protobuf::util::TypeResolver> resolver{
      google::protobuf::util::NewTypeResolverForDescriptorPool(
          type_prefix, output->GetDescriptor()->file()->pool())};
  std::string binary_proto;
  // The JSON stream parser doesn't accept an empty string.
  const void* data;
  int bytes;
  if (json_stream->Next(&data, &bytes)) {
    json_stream->BackUp(bytes);
    google::protobuf::io::StringOutputStream binary_stream{&binary_proto};
    if (absl::Status json_parsed =
            AsAbslStatus(google::protobuf::util::JsonToBinaryStream(
                resolver.get(), type_url, json_stream, &binary_stream));
        !json_parsed.ok()) {
      return json_parsed;
    }
  }
  if (!output->MergeFromString(binary_proto)) {
    return absl::InternalError(
        "JSON transcoder produced invalid protobuf output.");
  }
  return absl::OkStatus();
}

// Parses a JSON file containing a proto message
absl::Status JsonFileToMessage(const char* file_path, google::protobuf::Message* output) {
  int defaults_fd = open(file_path, O_RDONLY);
  if (defaults_fd < 0) {
    return absl::InvalidArgumentError(
        absl::StrCat("Failed to open defaults file: ", file_path));
  }
  output->Clear();
  google::protobuf::io::FileInputStream defaults_stream{defaults_fd};
  absl::Status status = MergeFromJsonStream(&defaults_stream, output);
  defaults_stream.Close();
  return status;
}

absl::Status GetParams(google::protobuf::Message* params) {
  if (isatty(STDIN_FILENO) && !getenv("OCPDIAG_STDIN")) {
    return absl::OkStatus();
  }
  params->Clear();
  google::protobuf::io::FileInputStream json_stream{STDIN_FILENO};
  return MergeFromJsonStream(&json_stream, params);
}

absl::string_view GetVersion() { return kVersionString; }

}  // namespace params
}  // namespace ocpdiag
