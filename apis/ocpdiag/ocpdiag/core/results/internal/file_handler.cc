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

#include "ocpdiag/core/results/internal/file_handler.h"

#include <filesystem>  //
#include <fstream>
#include <ios>
#include <memory>
#include <system_error>  //

#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "ocpdiag/core/hwinterface/lib/off_dut_machine_interface/remote_factory.h"

namespace ocpdiag {
namespace results {
namespace internal {

using hwinterface::remote::ConnInterface;
using ocpdiag::results_pb::File;

absl::StatusOr<std::unique_ptr<ConnInterface>> FileHandler::GetConnInterface(
    const std::string& node_addr) {
  absl::StatusOr<std::unique_ptr<hwinterface::remote::ConnInterface>> node =
      ocpdiag::hwinterface::remote::NewConn(
          hwinterface::remote::NodeSpec{.address = node_addr});
  if (!node.ok()) {
    return absl::UnavailableError(
        absl::StrFormat("Could not establish connection to remote node "
                        "%s for file transfer: %s",
                        node_addr, node.status().message()));
  }
  return node;
}

absl::StatusOr<std::unique_ptr<std::ostream>> FileHandler::OpenLocalFile(
    absl::string_view local_filename) {
  auto output = std::make_unique<std::ofstream>();
  // overrite file if exists.
  output->open(std::string(local_filename),
               std::ios_base::out | std::ios_base::trunc);
  if (!output->is_open()) {
    return absl::UnknownError(
        absl::StrFormat("Could not open local file %s", local_filename));
  }
  return std::move(output);
}

absl::Status FileHandler::CopyRemoteFile(File& file) {
  absl::StatusOr<std::unique_ptr<hwinterface::remote::ConnInterface>> node =
      GetConnInterface(file.node_address());

  absl::string_view filepath = file.output_path();
  absl::StatusOr<absl::Cord> data = node.value()->ReadFile(filepath);
  if (!data.ok()) {
    return absl::UnknownError(absl::StrFormat(
        "Failed to read remote file on node %s with file path %s : %s",
        file.node_address(), filepath, data.status().message()));
  }
  // Set local filename to a combination of absolute path on remote node +
  // node address: ex. /tmp/data/output on "node1" -> node1._tmp_data_output
  std::string local_filename = absl::StrCat(
      file.node_address(), ".", std::string(std::filesystem::path{filepath}));
  local_filename = absl::StrReplaceAll(local_filename, {{"/", "_"}});
  // Overwrite some proto fields
  if (file.upload_as_name().empty()) {
    file.set_upload_as_name(local_filename);
  }
  file.set_output_path(local_filename);
  absl::StatusOr<std::unique_ptr<std::ostream>> output =
      OpenLocalFile(local_filename);
  if (!output.ok()) {
    return output.status();
  }
  **output << *data;
  (*output)->flush();
  return absl::OkStatus();
}

absl::Status FileHandler::CopyLocalFile(File& file,
                                        absl::string_view dest_dir) {
  const std::string src = file.output_path();
  std::string dest = absl::StrCat(
      dest_dir, std::string(std::filesystem::path{src}.filename()), "_copy");
  std::error_code err;
  std::filesystem::copy(src, dest,
                        std::filesystem::copy_options::overwrite_existing, err);
  if (err) {
    return absl::UnknownError(
        absl::StrFormat("Failed to copy file %s: %s", src, err.message()));
  }
  file.set_output_path(dest);
  return absl::OkStatus();
}

}  // namespace internal
}  // namespace results
}  // namespace ocpdiag
