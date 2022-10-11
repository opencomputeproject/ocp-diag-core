// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/output_receiver.h"

#include <cstdlib>
#include <filesystem>  //
#include <memory>
#include <optional>

#include "absl/log/check.h"
#include "absl/strings/str_format.h"
#include "ocpdiag/core/results/recordio_iterator.h"
#include "ocpdiag/core/results/results.pb.h"

namespace ocpdiag::results {

using ::ocpdiag::results_pb::OutputArtifact;

OutputReceiver::OutputReceiver()
    : file_path_([] {
        // Try several options for a temporary filename until a suitable
        // one is found. Note that the tempdir may be different depending on
        // environment.
        std::string path = std::getenv("TEST_TMPDIR");
        if (path.empty()) path = std::getenv("TMPDIR");
        if (path.empty()) path = std::filesystem::temp_directory_path();
        path += "/ocpdiag_output_receiver_tempfile_XXXXXX";
        CHECK_NE(mkstemp(path.data()), -1) << "Cannot make temp file path";

        // Prepare the output file to receive test data.
        if (std::filesystem::exists(path))
          CHECK(std::filesystem::remove(path)) << "Cannot remove temp file";
        return path;
      }()),
      artifact_writer_([&] {
        // Create an artifact writer that outputs to a file, as well as stdout
        // for easier examination during unit tests.
        absl::StatusOr<int> fd = internal::OpenAndGetDescriptor(file_path_);
        CHECK_OK(fd.status()) << "Cannot open file";
        bool log_to_stdout = true;
        return std::make_unique<internal::ArtifactWriter>(*fd, log_to_stdout);
      }()) {}

const TestRunOutput &OutputReceiver::model() {
  // Populate the test run output model the first time this is called.
  if (!model_.has_value()) {
    model_ = TestRunOutput{};
    for (OutputArtifact artifact : *this) {
      CHECK_OK(AddOutputArtifact(*model_, std::move(artifact)));
    }
  }
  return *model_;
}

OutputReceiver::const_iterator OutputReceiver::begin() const {
  return RecordIoIterator<OutputArtifact>(file_path_);
}

OutputReceiver::const_iterator OutputReceiver::end() const {
  return RecordIoIterator<OutputArtifact>();
}

}  // namespace ocpdiag::results
