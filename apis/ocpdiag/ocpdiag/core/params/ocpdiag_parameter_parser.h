// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_PARAMS_OCPDIAG_PARAMETER_PARSER_H_
#define OCPDIAG_CORE_PARAMS_OCPDIAG_PARAMETER_PARSER_H_

#include <string>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace ocpdiag {

// This class wraps parameter parsing functions, mostly for unit testing.
// The actual use is as a standalone binary.
class OCPDiagParameterParser {
 public:
  // The positional arguments expected by the parameter parser.
  enum OCPDiagArgs {
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

}  // namespace ocpdiag

#endif  // OCPDIAG_CORE_PARAMS_OCPDIAG_PARAMETER_PARSER_H_
