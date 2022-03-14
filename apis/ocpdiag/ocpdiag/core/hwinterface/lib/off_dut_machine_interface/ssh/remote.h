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

#ifndef OCPDIAG_CORE_HWINTERFACE_LIB_OFF_DUT_MACHINE_INTERFACE_SSH_REMOTE_H_
#define OCPDIAG_CORE_HWINTERFACE_LIB_OFF_DUT_MACHINE_INTERFACE_SSH_REMOTE_H_

#include <memory>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "ocpdiag/core/hwinterface/lib/off_dut_machine_interface/remote.h"

namespace ocpdiag::hwinterface {
namespace remote {

// SSHConnInterface implements ConnInterface defined in
// ocpdiag/core/hwinterface/lib/off_dut_machine_interface/remote.h
class SSHConnInterface : public ConnInterface {
 public:
  // Create a connection to the given node.
  explicit SSHConnInterface(NodeSpec node_spec, absl::string_view ssh_key_path,
                            absl::string_view ssh_tunnel_file_path,
                            absl::string_view ssh_bin_path);

  ~SSHConnInterface() override = default;

  // ReadFile reads the given file.
  absl::StatusOr<absl::Cord> ReadFile(absl::string_view file_name) override;

  // WriteFile writes data to the given file.
  absl::Status WriteFile(absl::string_view file_name,
                         absl::string_view data) override;

  // RunCommand runs the specified command on the given machine node.
  absl::StatusOr<CommandResult> RunCommand(
      absl::Duration timeout, const absl::Span<const absl::string_view> args,
      const CommandOption& options) override;

 private:
  absl::StatusOr<CommandResult> RunCommandWithStdin(
      absl::Duration timeout, absl::string_view stdin,
      absl::Span<const absl::string_view> args, const CommandOption& options);
  std::vector<std::string> GenerateSshArg(
      absl::Span<const absl::string_view> args);

  NodeSpec node_spec_;
  std::string ssh_key_path_;
  std::string ssh_tunnel_file_path_;
  std::string ssh_bin_path_;
};

}  // namespace remote
}  // namespace ocpdiag::hwinterface

#endif  // OCPDIAG_CORE_HWINTERFACE_LIB_OFF_DUT_MACHINE_INTERFACE_SSH_REMOTE_H_
