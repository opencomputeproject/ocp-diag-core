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

#ifndef OCPDIAG_CORE_RESULTS_INTERNAL_FILE_HANDLER_H_
#define OCPDIAG_CORE_RESULTS_INTERNAL_FILE_HANDLER_H_

#include <memory>
#include <ostream>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "ocpdiag/core/hwinterface/lib/off_dut_machine_interface/remote.h"
#include "ocpdiag/core/results/results.pb.h"

namespace ocpdiag {
namespace results {
namespace internal {

constexpr absl::string_view kWorkingDir = "";

// Handles the copying of remote and local file artifacts for a OCPDiag test.
class FileHandler {
 public:
  FileHandler() = default;
  virtual ~FileHandler() = default;

  // Returns a connection to a remote node, if available.
  virtual absl::StatusOr<std::unique_ptr<hwinterface::remote::ConnInterface>>
  GetConnInterface(const std::string& node_addr);

  // Opens a file on the local filesystem and return as an ostream.
  // NOTE: Overwrites the file contents if it already exists.
  virtual absl::StatusOr<std::unique_ptr<std::ostream>> OpenLocalFile(
      absl::string_view local_filename);

  // Copies a file on a remote node to a locally created file.
  // Modifies `file` parameter.
  virtual absl::Status CopyRemoteFile(
      ocpdiag::results_pb::File& file);

  // Copies a file on localhost to working directory.
  // Modifies `file` parameter.
  virtual absl::Status CopyLocalFile(
      ocpdiag::results_pb::File& file,
      absl::string_view dest_dir = kWorkingDir);
};

}  // namespace internal
}  // namespace results
}  // namespace ocpdiag

#endif  // OCPDIAG_CORE_RESULTS_INTERNAL_FILE_HANDLER_H_
