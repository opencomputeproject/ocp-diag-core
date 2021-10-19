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

#ifndef MELTAN_LIB_MELTAN_DIAGNOSTIC_H_
#define MELTAN_LIB_MELTAN_DIAGNOSTIC_H_

#include <cstdlib>
#include <memory>

#include "absl/memory/memory.h"
#include "meltan/lib/results/results.h"

namespace meltan {

// MeltanDiagnostic is an abstract helper interface to unify some of the
// organization common to all diagnostic tests written using the Meltan APIs.
class MeltanDiagnostic {
 public:
  explicit MeltanDiagnostic(meltan::results::ResultApi* api = nullptr) {
    api_ = api;
    if (api_ == nullptr) {
      default_api_ = absl::make_unique<meltan::results::ResultApi>();
      api_ = default_api_.get();
    }
  }
  virtual ~MeltanDiagnostic() = default;

  // Common entry point for executing Meltan diagnostic tests.
  // Must be overridden.
  virtual int ExecuteTest() = 0;

  meltan::results::ResultApi* api() { return api_; }

 private:
  meltan::results::ResultApi* api_;
  std::unique_ptr<meltan::results::ResultApi> default_api_;
};

}  // namespace meltan

#endif  // MELTAN_LIB_MELTAN_DIAGNOSTIC_H_
