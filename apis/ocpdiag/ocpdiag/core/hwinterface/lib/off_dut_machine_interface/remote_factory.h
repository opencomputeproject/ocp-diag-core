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

#ifndef OCPDIAG_CORE_HWINTERFACE_LIB_OFF_DUT_MACHINE_INTERFACE_REMOTE_FACTORY_H_
#define OCPDIAG_CORE_HWINTERFACE_LIB_OFF_DUT_MACHINE_INTERFACE_REMOTE_FACTORY_H_

#include "absl/flags/declare.h"
#include "ocpdiag/core/hwinterface/lib/off_dut_machine_interface/remote.h"

// machine interface type.
ABSL_DECLARE_FLAG(std::string, mi_connection_type);
// remote_ssh_key_path is for ovss cbtr which uses a private key.
// alternatively, remote_ssh_tunnel_file_path can be used to improve the
// efficiency.
ABSL_DECLARE_FLAG(std::string, remote_ssh_key_path);
// remote_ssh_tunnel_file_path is similar to how OpenTest reuse the ssh
// connections. Check SSH Multiplexing for more details.
ABSL_DECLARE_FLAG(std::string, remote_ssh_tunnel_file_path);

namespace ocpdiag::hwinterface {
namespace remote {

enum class ConnectionTypes {
  kProd = 0,
  kSSH,
};

// NewConn creates a new machine node connection
// to read/write remote files, or run remote commands
// on the specified machine node.
absl::StatusOr<std::unique_ptr<ConnInterface>> NewConn(NodeSpec node_spec);

// GetSSHPath returns the path to the SSH binary.
absl::StatusOr<std::string> GetSSHPath(absl::string_view path_env);

}  // namespace remote
}  // namespace ocpdiag::hwinterface

#endif  // OCPDIAG_CORE_HWINTERFACE_LIB_OFF_DUT_MACHINE_INTERFACE_REMOTE_FACTORY_H_
