// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.


#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>

#include <cstdio>
#include <cstring>
#include <iostream>

#include "ocpdiag/core/params/testdata/dummy_outputs.h"

// Binary dumps its argv and stdin to the files in dummy_outputs.h.
int main(int argc, char* argv[]) {
  const char* argfile = getenv(ocpdiag::kDummyBinaryArgsFileEnvVar);
  const char* stdinfile = getenv(ocpdiag::kDummyBinaryStdinEnvVar);
  if (argfile == nullptr || stdinfile == nullptr) {
    std::cerr << "Dummy binary must define the environment variables "
              << ocpdiag::kDummyBinaryArgsFileEnvVar << " and "
              << ocpdiag::kDummyBinaryStdinEnvVar << std::endl;
    return 1;
  }
  FILE* execv = std::fopen(argfile, "w+");
  if (!execv) {
    return 1;
  }
  FILE* input = std::fopen(stdinfile, "w+");
  if (!input) {
    return 1;
  }
  int retval = 0;
  for (int i = 0; i < argc; ++i) {
    size_t len = strlen(argv[i]) + 1;
    if (std::fwrite(argv[i], sizeof(char), len, execv) != len) {
      return 1;
    }
    if (strcmp(argv[i], "--help") == 0) {
      retval = 2;  // Mirror the absl --help library and return 2.
    }
  }
  char buf[2048];
  ssize_t read;
  do {
    read = std::fread(buf, sizeof(char), sizeof(buf) / sizeof(char), stdin);
    if (read < 0 || std::fwrite(buf, sizeof(char), read, input) !=
                        static_cast<size_t>(read)) {
      return 1;
    }
  } while (read > 0);
  return retval;
}
