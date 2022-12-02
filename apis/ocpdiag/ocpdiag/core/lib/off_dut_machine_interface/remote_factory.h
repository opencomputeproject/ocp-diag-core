// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_HWINTERFACE_LIB_OFF_DUT_MACHINE_INTERFACE_REMOTE_FACTORY_H_
#define OCPDIAG_CORE_HWINTERFACE_LIB_OFF_DUT_MACHINE_INTERFACE_REMOTE_FACTORY_H_

#include "absl/flags/declare.h"
#include "ocpdiag/core/lib/off_dut_machine_interface/remote.h"

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
