// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_PARAMS_FAKE_PARAMS_H_
#define OCPDIAG_CORE_PARAMS_FAKE_PARAMS_H_

#include "google/protobuf/message.h"
#include "absl/status/statusor.h"

namespace ocpdiag {
namespace params {

// absl::Cleanup can't be explicitly returned, so this replicates it using a
// std::function for callback type erasure. Less efficient, but it's test-only.
class ParamsCleanup final {
 public:
  ParamsCleanup(std::function<void()> cleanup) : cleanup_(std::move(cleanup)) {}
  ParamsCleanup(ParamsCleanup&& from) : cleanup_(std::move(from.cleanup_)) {
    from.cleanup_ = []() {};
  }
  ~ParamsCleanup() { Cleanup(); }

  // It is not required to call this explicitly, it will be called automatically
  // in the destructor. This function is exposed for Python wrappers only.
  void Cleanup();

 private:
  std::function<void()> cleanup_;
};

absl::StatusOr<ParamsCleanup> FakeParams(const google::protobuf::Message& params);

}  // namespace params
}  // namespace ocpdiag

#endif  // OCPDIAG_CORE_PARAMS_FAKE_PARAMS_H_
