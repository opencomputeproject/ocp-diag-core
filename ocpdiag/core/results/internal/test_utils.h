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

#ifndef MELTAN_LIB_RESULTS_INTERNAL_TEST_UTILS_H_
#define MELTAN_LIB_RESULTS_INTERNAL_TEST_UTILS_H_

#include <fcntl.h>

#include "gtest/gtest.h"

namespace ocpdiag {
namespace results {
namespace internal {

// RAII struct that creates a temp file or FAILs the test if not.
// Then closes the file descriptor when it falls out of scope.
struct TestFile {
  TestFile() {
    ptr = std::tmpfile();
    Verify();
    descriptor = fileno(ptr);
  }
  void Verify() {
    if (ptr == nullptr) {
      FAIL() << "failed to create temp file.";
    }
  }
  ~TestFile() {
    if (ptr != nullptr) {
      fclose(ptr);
    }
  }
  FILE* ptr;
  int descriptor;
};

}  // namespace internal
}  // namespace results
}  // namespace ocpdiag

#endif  // MELTAN_LIB_RESULTS_INTERNAL_TEST_UTILS_H_
