// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_LIB_RESULTS_INTERNAL_TEST_UTILS_H_
#define OCPDIAG_LIB_RESULTS_INTERNAL_TEST_UTILS_H_

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

#endif  // OCPDIAG_LIB_RESULTS_INTERNAL_TEST_UTILS_H_
