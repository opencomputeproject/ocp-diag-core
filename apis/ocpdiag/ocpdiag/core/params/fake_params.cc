// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/params/fake_params.h"

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
#include "ocpdiag/core/compat/status_converters.h"

namespace ocpdiag {
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
}  // namespace ocpdiag
