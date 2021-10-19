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

// This program is used to bootstrap meltan diags.
// It consumes JSON data from stdin, merges it with the optional JSON
// defaults file, then overrides any values from command-line flags, and finally
// execs another command passing it the resulting JSON object on stdin and
// any unused flags as arguments. If there is an issue, or the execed task exits
// before consuming stdin, errors will be printed on stderr and a non-zero value
// returned.

#include <stddef.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "meltan/lib/params/meltan_parameter_parser.h"
#include "meltan/lib/params/utils.h"

namespace {

// ExecAndFeed writes json_params to a tmpfile, assigns it to stdin, and then
// execs the test executable (i.e. exec_args[0]). It will not return on success.
absl::Status ExecAndFeed(absl::Span<const char* const> exec_args,
                         absl::string_view json_params) {
  FILE* file = std::tmpfile();
  if (file == nullptr) {
    return absl::InternalError("Failed to create params input file.");
  }
  while (!json_params.empty()) {
    size_t written =
        std::fwrite(json_params.data(), sizeof(char), json_params.size(), file);
    if (written < 0) {
      return absl::InternalError("Failed to write params to temporary file.");
    }
    json_params.remove_prefix(written);
  }
  std::rewind(file);
  std::fflush(file);
  if (dup2(fileno(file), STDIN_FILENO) < 0) {
    return absl::InternalError("Failed to assign params file to stdin.");
  }
  std::fclose(file);
  execv(exec_args[0], const_cast<char* const*>(exec_args.data()));
  return absl::InternalError(
      absl::StrCat("execv(\"", exec_args[0], "\", args...) failed to run."));
}

absl::StatusOr<bool> GetBooleanFlag(
    absl::string_view name, bool default_value,
    const meltan::MeltanParameterParser::Arguments& args) {
  for (const meltan::MeltanParameterParser::FlagArg& flag : args.flags) {
    if (flag.key == name) {
      bool value = default_value;
      if (flag.value.empty()) {
        return true;
      }
      if (!absl::SimpleAtob(flag.value, &value)) {
        return absl::InvalidArgumentError(absl::StrCat(
            "Could not parse \"", flag.value, "\" to a boolean value."));
      }
      return value;
    }
  }
  return default_value;
}

// Detect if the current terminal environment is dumb.
bool DumbTerm() {
  const char* term_env = getenv("TERM");
  if (term_env == NULL) {  // OSS string_view doesn't handle NULL strings.
    return true;
  }
  absl::string_view term = term_env;
  static constexpr const char* dumb_terms[] = {"", "dumb", "unknown"};
  for (const char* dumb_term : dumb_terms) {
    if (term == dumb_term) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main(int argc, const char* argv[]) {
  std::unique_ptr<google::protobuf::io::ZeroCopyInputStream> json_stream =
      std::make_unique<google::protobuf::io::ArrayInputStream>("", 0);
  // Read from stdin if running in a terminal but stdin has been replaced,
  // or stdin was explicitly requested.
  if ((!isatty(STDIN_FILENO) && !DumbTerm()) || getenv("MELTAN_STDIN")) {
    json_stream = std::make_unique<google::protobuf::io::FileInputStream>(STDIN_FILENO);
  }
  meltan::MeltanParameterParser::Arguments args =
      meltan::MeltanParameterParser::ParseArgs(argc, argv);
  absl::StatusOr<bool> dry_run = GetBooleanFlag("dry_run", false, args);
  if (!dry_run.ok()) {
    std::cerr << dry_run.status();
    return 1;
  }
  absl::StatusOr<bool> version = GetBooleanFlag("version", false, args);
  if (!version.ok()) {
    std::cerr << version.status();
    return 1;
  }
  if (*version) {
    std::cout << "Version: " << meltan::params::GetVersion() << std::endl;
    return 2;
  }
  absl::StatusOr<meltan::MeltanParameterParser::ExecArgs> exec_args =
      meltan::MeltanParameterParser::PrepareExec(std::move(args),
                                                 json_stream.get(), *dry_run);
  if (!exec_args.ok()) {
    std::cerr << exec_args.status() << std::endl;
    return 1;
  }
  if (*dry_run) {
    // The exec args always end with a nullptr.
    exec_args->execv.pop_back();
    std::cerr << "This test was started with --dry_run."
              << "If it was actually run, the raw arguments would have been \n"
              << absl::StrJoin(exec_args->execv, " ")
              << "\nIt would be passed the parameters via stdin." << std::endl;
    // Put the JSON params on stdout, so shell redirection doesn't capture
    // the human-readable text.
    std::cout << exec_args->json_params << std::endl;
    return 2;
  }
  // Fork the test in a subprocess if any post-test output is requested.
  if (!exec_args->post_output.empty()) {
    pid_t pid = fork();
    if (pid < 0) {
      std::cerr << "Failed to fork." << std::endl;
      return 1;
    }
    if (pid != 0) {
      int wstatus;
      waitpid(pid, &wstatus, 0);
      if (WIFEXITED(wstatus)) {
        std::cout << exec_args->post_output << std::endl;
        return WEXITSTATUS(wstatus);
      }
      std::cerr << "Test exited abnormally." << std::endl;
      return 1;
    }
  }
  // ExecAndFeed doesn't return on success.
  std::cerr << ExecAndFeed(absl::MakeSpan(exec_args->execv),
                           exec_args->json_params)
            << std::endl;
  return 2;
}
