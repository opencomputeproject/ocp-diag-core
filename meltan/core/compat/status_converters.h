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

#ifndef MELTAN_CORE_COMPAT_STATUS_CONVERTERS_H_
#define MELTAN_CORE_COMPAT_STATUS_CONVERTERS_H_

#include "grpcpp/impl/codegen/status.h"
#include "absl/status/status.h"

namespace meltan {

// Compatibility method for differences between internal/external
// gRPC and abseil status libraries. In google, the two statuses can
// be implicitly cast to each other, externally that causes build breakage.

inline grpc::Status ToGRPCStatus(const absl::Status& status) {
  return grpc::Status(
  static_cast<grpc::StatusCode>(status.code()),
  std::string(status.message()));
}

// Conversion helpers between abseil and open-source protobuf status. In google,
// all statuses are abseil so the specialization should always be invoked.
template <typename T>
absl::Status AsAbslStatus(const T& status) {
  static_assert(!std::is_same_v<decltype(status), absl::Status>);
  return absl::Status(absl::StatusCode(static_cast<int>(status.code())),
                      status.message().as_string());
}
// Equivalent for gRPC status.
template <>
inline absl::Status AsAbslStatus(const grpc::Status& status) {
  return absl::Status(absl::StatusCode(static_cast<int>(status.error_code())),
                      status.error_message());
}
// Internal specialization
template <>
inline absl::Status AsAbslStatus<absl::Status>(const absl::Status& status) {
  return status;
}

}  // namespace meltan
#endif  // MELTAN_CORE_COMPAT_STATUS_CONVERTERS_H_
