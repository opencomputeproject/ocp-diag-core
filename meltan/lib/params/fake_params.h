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

#ifndef MELTAN_LIB_PARAMS_FAKE_PARAMS_H_
#define MELTAN_LIB_PARAMS_FAKE_PARAMS_H_

#include "google/protobuf/message.h"
#include "absl/status/statusor.h"

namespace meltan {
namespace params {

// absl::Cleanup can't be explicitly returned, so this replicates it using a
// std::function for callback type erasure. Less efficient, but it's test-only.
class ParamsCleanup final {
 private:
  std::function<void()> cleanup_;

 public:
  ParamsCleanup(std::function<void()> cleanup) : cleanup_(std::move(cleanup)) {}
  ParamsCleanup(ParamsCleanup&& from) : cleanup_(std::move(from.cleanup_)) {
    from.cleanup_ = []() {};
  }
  ~ParamsCleanup() { cleanup_(); }
};

absl::StatusOr<ParamsCleanup> FakeParams(const google::protobuf::Message& params);

}  // namespace params
}  // namespace meltan

#endif  // MELTAN_LIB_PARAMS_FAKE_PARAMS_H_
