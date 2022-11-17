// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_HWINTERFACE_LIB_OFF_DUT_MACHINE_INTERFACE_REMOTE_H_
#define OCPDIAG_CORE_HWINTERFACE_LIB_OFF_DUT_MACHINE_INTERFACE_REMOTE_H_

#include "absl/status/statusor.h"
#include "absl/time/time.h"

namespace ocpdiag::hwinterface {
namespace remote {

struct NodeSpec {
  std::string address;
};

// Class ConnInterface provides a remote connection to the specified machine
// node. It provides the file read/write operations, and the capability to
// launch a remote command on the machine node.
class ConnInterface {
 public:
  // Options to configure a command.
  struct CommandOption {
    // The following arguments specify an absolute file path for redirecting
    // stdout/stderr. Whenever the stdout/stderr is redirected, the
    // corresponding field in "CommandResult" will be empty.
    //
    std::string stdout_file;
    std::string stderr_file;
  };

  // The exit code and the command's output to stdout and stderr.
  struct CommandResult {
    // set to -127 by default.
    // exit_code = 0 means OK. follows the python-style exit codes.
    int exit_code = -127;
    std::string stdout;
    std::string stderr;
  };

  virtual ~ConnInterface() = default;

  // ReadFile reads a file from the machine node, and returns the full file
  // content on success, or the error status when applicable.
  virtual absl::StatusOr<absl::Cord> ReadFile(absl::string_view file_name) = 0;

  // WriteFile writes the given data to the file on the machine node and returns
  // the status.
  virtual absl::Status WriteFile(absl::string_view file_name,
                                 absl::string_view data) = 0;

  // RunCommand runs a remote command on the machine node, and returns the
  // command output on success, or the error status when applicable.
  // If the command's stdout/stderr is redirected by setting the CommandOption
  // option, the corresponding field in "CommandResult" will be empty.
  virtual absl::StatusOr<CommandResult> RunCommand(
      absl::Duration timeout, const absl::Span<const absl::string_view> args,
      const CommandOption& options) = 0;
};

}  // namespace remote
}  // namespace ocpdiag::hwinterface

#endif  // OCPDIAG_CORE_HWINTERFACE_LIB_OFF_DUT_MACHINE_INTERFACE_REMOTE_H_
