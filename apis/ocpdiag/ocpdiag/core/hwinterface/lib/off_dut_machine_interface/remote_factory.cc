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

#include "ocpdiag/core/hwinterface/lib/off_dut_machine_interface/remote_factory.h"

#include <stdlib.h>

#include <cctype>
#include <filesystem>
#include <string_view>
#include <unordered_set>

#include "absl/container/flat_hash_map.h"
#include "absl/flags/flag.h"
#include "absl/status/status.h"
#include "absl/strings/ascii.h"
#include "ocpdiag/core/hwinterface/lib/off_dut_machine_interface/remote.h"
#include "ocpdiag/core/hwinterface/lib/off_dut_machine_interface/ssh/remote.h"

ABSL_FLAG(std::string, mi_connection_type, "",
          "The type of Machine Interface to use. Current implementations are "
          "prod, ssh. Defaults to SSH.");
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

// Using flat_hash_map as gtl::fixed_flat_map_of is unavailable for Copy.Bara.
static const auto* const kStringToConnectionTypeMap =
    new absl::flat_hash_map<absl::string_view, ConnectionTypes>({
        {"ssh", ConnectionTypes::kSSH},
        {"", ConnectionTypes::kSSH},
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
      return absl::make_unique<SSHConnInterface>(
          node_spec, absl::GetFlag(FLAGS_remote_ssh_key_path),
          absl::GetFlag(FLAGS_remote_ssh_tunnel_file_path), *ssh_bin);
  }
}

}  // namespace remote
}  // namespace ocpdiag::hwinterface
