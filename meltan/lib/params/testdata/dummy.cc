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


#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>

#include <cstdio>
#include <cstring>
#include <iostream>

#include "meltan/lib/params/testdata/dummy_outputs.h"

// Binary dumps its argv and stdin to the files in dummy_outputs.h.
int main(int argc, char* argv[]) {
  const char* argfile = getenv(meltan::kDummyBinaryArgsFileEnvVar);
  const char* stdinfile = getenv(meltan::kDummyBinaryStdinEnvVar);
  if (argfile == nullptr || stdinfile == nullptr) {
    std::cerr << "Dummy binary must define the environment variables "
              << meltan::kDummyBinaryArgsFileEnvVar << " and "
              << meltan::kDummyBinaryStdinEnvVar << std::endl;
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
