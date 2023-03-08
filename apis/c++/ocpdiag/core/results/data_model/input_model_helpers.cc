// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/data_model/input_model_helpers.h"

#include <string>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"

namespace ocpdiag::results {

std::string CommandLineStringFromMainArgs(int argc, const char* argv[]) {
  std::string command_line = argv[0];
  for (int i = 1; i < argc; i++) absl::StrAppend(&command_line, " ", argv[i]);
  return command_line;
}

std::string ParameterJsonFromMainArgs(int argc, const char* argv[]) {
  if (argc <= 1) return "{}";

  std::string parameter_json = "{";
  for (int i = 1; i < argc; i += 2) {
    while (argv[i][0] == '-') argv[i]++;
    absl::StrAppend(&parameter_json,
                    absl::StrFormat("\"%s\":\"%s\"", argv[i], argv[i + 1]));
    if (i + 2 < argc) absl::StrAppend(&parameter_json, ",");
  }
  absl::StrAppend(&parameter_json, "}");
  return parameter_json;
}

}  // namespace ocpdiag::results
