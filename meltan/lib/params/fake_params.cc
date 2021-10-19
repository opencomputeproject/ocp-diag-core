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

#include "meltan/lib/params/fake_params.h"

#include <unistd.h>

#include <memory>
#include <string>

#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/message.h"
#include "google/protobuf/util/json_util.h"
#include "google/protobuf/util/type_resolver.h"
#include "google/protobuf/util/type_resolver_util.h"
#include "absl/cleanup/cleanup.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "meltan/core/compat/status_converters.h"

namespace meltan {
namespace params {

absl::StatusOr<ParamsCleanup> FakeParams(const google::protobuf::Message& params) {
  const std::string type_prefix = "type.googleapis.com";
  const std::string type_url =
      absl::StrCat(type_prefix, "/", params.GetDescriptor()->full_name());
  std::unique_ptr<google::protobuf::util::TypeResolver> resolver{
      google::protobuf::util::NewTypeResolverForDescriptorPool(
          type_prefix, params.GetDescriptor()->file()->pool())};
  std::string binary = params.SerializeAsString();
  FILE* input = std::tmpfile();
  if (input == nullptr) {
    return absl::UnknownError("Failed to create temporary file for parameters");
  }
  auto closer = absl::MakeCleanup([input]() { std::fclose(input); });
  google::protobuf::io::ArrayInputStream reader(binary.data(), binary.size());
  google::protobuf::io::FileOutputStream writer(fileno(input));
  absl::Status written = AsAbslStatus(google::protobuf::util::BinaryToJsonStream(
      resolver.get(), type_url, &reader, &writer));
  if (!written.ok()) {
    return written;
  }
  writer.Flush();
  std::rewind(input);
  int saved_stdin = dup(STDIN_FILENO);
  dup2(fileno(input), STDIN_FILENO);
  return ParamsCleanup([saved_stdin]() {
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
  });
}

}  // namespace params
}  // namespace meltan
