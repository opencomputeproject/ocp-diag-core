// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/lib/off_dut_machine_interface/remote_factory.h"

#include <stdlib.h>

#include <cctype>
#include <filesystem>
#include <memory>
#include <string_view>
#include <unordered_set>

#include "absl/container/flat_hash_map.h"
#include "absl/flags/flag.h"
#include "absl/status/status.h"
#include "absl/strings/ascii.h"
#include "ocpdiag/core/lib/off_dut_machine_interface/remote.h"
#include "ocpdiag/core/lib/off_dut_machine_interface/ssh/remote.h"

ABSL_FLAG(std::string, mi_connection_type, "ssh",
          "The type of Machine Interface to use. Current implementations are "
          "prod, ssh. Defaults to ssh.");
ABSL_FLAG(std::string, mi_service_addr, "",
          "Not Yet Implemented. Machine "
          "interface service address, for Google prod.");
ABSL_FLAG(std::string, remote_ssh_key_path, "",
          "in case the ssh connection needs a private key, for MTP/vendor.");
ABSL_FLAG(std::string, remote_ssh_tunnel_file_path, "",
          "ssh multiplex to improve the efficiency, for MTP/vendor.");

namespace ocpdiag::hwinterface {
namespace remote {

constexpr absl::string_view kBin = "/bin/ssh";
constexpr absl::string_view kUsr = "/usr/bin/ssh";
constexpr absl::string_view kSshCommand = "ssh";

absl::StatusOr<std::string> GetSSHPath(absl::string_view path_env) {
  std::unordered_set<std::string> ssh_paths = {kBin.data(), kUsr.data()};
  std::string sep = ":";
  size_t cur;
  size_t start = 0;

  while ((cur = path_env.find(sep, start)) != std::string::npos) {
    std::filesystem::path current_path(path_env.substr(start, cur - start));
    current_path = current_path / kSshCommand;

    if (ssh_paths.find(current_path) != ssh_paths.end() &&
        std::filesystem::exists(current_path)) {
      return current_path.string();
    }
    start = cur + 1;
  }
  return absl::NotFoundError("Unable to find valid SSH Path");
}

static const auto* const kStringToConnectionTypeMap =
    new absl::flat_hash_map<absl::string_view, ConnectionTypes>({
        {"ssh", ConnectionTypes::kSSH},
    });

absl::StatusOr<std::unique_ptr<ConnInterface>> NewConn(NodeSpec node_spec) {
  if (node_spec.address.empty()) {
    return absl::InvalidArgumentError(
        "Machine node address must not be empty.");
  }
  auto mi_connection_type_str =
      absl::AsciiStrToLower(absl::GetFlag(FLAGS_mi_connection_type));
  auto mi_connection_type =
      kStringToConnectionTypeMap->find(mi_connection_type_str);
  if (mi_connection_type == kStringToConnectionTypeMap->end()) {
    return absl::InvalidArgumentError(absl::StrCat(
        mi_connection_type_str, " is an invalid connection type."));
  }

  switch (mi_connection_type->second) {
    default:
      std::string path_env = getenv("PATH");
      absl::StatusOr<std::string> ssh_bin = GetSSHPath(path_env);
      if (!ssh_bin.ok()) {
        return ssh_bin.status();
      }
      return std::make_unique<SSHConnInterface>(
          node_spec, absl::GetFlag(FLAGS_remote_ssh_key_path),
          absl::GetFlag(FLAGS_remote_ssh_tunnel_file_path), *ssh_bin);
  }
}

}  // namespace remote
}  // namespace ocpdiag::hwinterface
