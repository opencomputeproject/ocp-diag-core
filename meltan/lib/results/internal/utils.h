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

#ifndef MELTAN_LIB_RESULTS_INTERNAL_UTILS_H_
#define MELTAN_LIB_RESULTS_INTERNAL_UTILS_H_

#include "absl/synchronization/mutex.h"

namespace meltan {
namespace results {
namespace internal {

// Threadsafe class that generates monotonically increasing integers,
// starting from zero. Values are not globally unique, but are unique amongst
// all users of a shared instance.
class IntIncrementer {
 public:
  IntIncrementer() : count_(0) {}
  IntIncrementer(IntIncrementer&& other) { *this = std::move(other); }
  IntIncrementer& operator=(IntIncrementer&& other) {
    if (this != &other) {
      absl::MutexLock l(&mutex_);
      count_ = std::move(other.count_);
    }
    return *this;
  }

  // Returns then increments count
  int Next() ABSL_LOCKS_EXCLUDED(mutex_) {
    absl::MutexLock l(&mutex_);
    return count_++;
  }

  // This class shall not allow reading the value of count_ without also
  // incrementing it.

 private:
  absl::Mutex mutex_;
  int count_ ABSL_GUARDED_BY(mutex_);
};

}  // namespace internal
}  // namespace results
}  // namespace meltan

#endif  // MELTAN_LIB_RESULTS_INTERNAL_UTILS_H_
