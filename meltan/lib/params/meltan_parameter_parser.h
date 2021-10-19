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

#ifndef MELTAN_LIB_PARAMS_MELTAN_PARAMETER_PARSER_H_
#define MELTAN_LIB_PARAMS_MELTAN_PARAMETER_PARSER_H_

#include <string>
#include <vector>

#include "google/protobuf/io/zero_copy_stream.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"

namespace meltan {

// This class wraps parameter parsing functions, mostly for unit testing.
// The actual use is as a standalone binary.
class MeltanParameterParser {
 public:
  // The positional arguments expected by the parameter parser.
  enum MeltanArgs {
    kLauncherName = 0,
    kTestExecutable,
    kFileDescriptors,
    kOptionalJSONDefaults,
    kMaxNumArgs,
  };

  // Represents a parsed flag. The key and value omit any switches ('-') and
  // separators ('='). Sources are the raw command-line flags that they came
  // from.
  struct FlagArg {
    absl::string_view key;
    absl::string_view value;
    absl::Span<const char* const> sources;
  };

  // The collection of parsed args, separated by type.
  // See below ParseArgs for an explanation of the different types.
  struct Arguments {
    absl::Span<const char* const> unparsed;
    absl::Span<const char* const> passthrough;
    std::vector<FlagArg> flags;
  };

  // Holds the arguments to pass to the exec-ed test.
  struct ExecArgs {
    // Has the executable as the first argument, and an ending NULL.
    std::vector<const char*> execv;
    // The merged JSON representation of the parameters to pipe to stdin.
    std::string json_params;
    // Extra output to std::cout after the program terminates.
    // If this is nonempty, then the program will fork a process to watch
    // and output this data.
    std::string post_output;
  };

  // ParseArgs parses the command line arguments into positional args, flags,
  // and passthrough args. Any arguments after a standalone '--' argument are
  // passthrough. Flag arguments start with '-' or '--' and have an optional
  // value in the following arg or separated by '=' in the same arg. All other
  // arguments are positional args.
  static Arguments ParseArgs(int argc, const char* argv[]);

  // PrepareExec uses the passed arguments to create the arguments needed to
  // exec the test. It determines what program to run, reads the parameter
  // descriptor, parses the optional defaults, merges the JSON from stdin over
  // the defaults, and then renders the final set of arguments and the JSON
  // parameters in the returned struct.
  static absl::StatusOr<ExecArgs> PrepareExec(
      Arguments args, google::protobuf::io::ZeroCopyInputStream* json_stream,
      bool json_newlines);
};

}  // namespace meltan

#endif  // MELTAN_LIB_PARAMS_MELTAN_PARAMETER_PARSER_H_
