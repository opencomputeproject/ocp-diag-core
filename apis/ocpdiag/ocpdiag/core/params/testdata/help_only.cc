// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.


#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/string_view.h"

ABSL_FLAG(std::string, dummy, "", "A flag with no meaning");

int main(int argc, char* argv[]) {
  absl::ParseCommandLine(argc, argv);
  return 0;
}
