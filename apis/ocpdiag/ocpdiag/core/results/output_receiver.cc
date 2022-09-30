// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/output_receiver.h"

#include <cstdlib>
#include <filesystem>  //
#include <memory>

#include "absl/base/attributes.h"
#include "absl/base/thread_annotations.h"
#include "absl/flags/flag.h"
#include "absl/functional/bind_front.h"
#include "absl/log/check.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "absl/synchronization/mutex.h"
#include "ocpdiag/core/results/internal/logging.h"
#include "ocpdiag/core/results/results.h"

namespace ocpdiag::results {
namespace {

void ClearFile(absl::string_view filename) {
  if (std::filesystem::exists(filename))
    CHECK(std::filesystem::remove(filename));
}

// Use a global variable to ensure we only have exactly one output receiver.
ABSL_CONST_INIT absl::Mutex mutex(absl::kConstInit);
bool singleton_initialized ABSL_GUARDED_BY(mutex) = false;

}  // namespace

OutputReceiver::OutputReceiver()
    : flag_saver_(std::make_unique<absl::FlagSaver>()), filename_([&] {
        // Try several options until we find a suitable one. The tempdir may be
        // different depending on environment.
        std::string path = std::getenv("TEST_TMPDIR");
        if (path.empty()) path = std::getenv("TMPDIR");
        if (path.empty()) path = std::filesystem::temp_directory_path();
        path += "/ocpdiag_output_receiver_tempfile_XXXXXX";
        CHECK_NE(mkstemp(path.data()), -1) << "Cannot create temp file";
        return path;
      }()) {
  {
    absl::MutexLock l(&mutex);
    CHECK(!singleton_initialized)
        << "Only one output receiver object can be instantiated at a time.";
    singleton_initialized = true;
  }

  // Prepare the output file to receive test data.
  ClearFile(filename_);
  absl::SetFlag(&FLAGS_ocpdiag_results_filepath, filename_);

  // Turn off stdout when intercepting the output.
  absl::SetFlag(&FLAGS_ocpdiag_copy_results_to_stdout, false);
}

void OutputReceiver::Close() {
  if (!flag_saver_) return;  // already closed
  flag_saver_.reset();

  absl::MutexLock l(&mutex);
  singleton_initialized = false;
}

TestRunOutput OutputReceiver::ConsumeOutputOrDie() {
  TestRunOutput output;
  Iterate([&](auto artifact) {
    CHECK_OK(output.AddOutputArtifact(std::move(artifact)));
    return true;
  });
  return output;
}

void OutputReceiver::Iterate(const OutputCallback &callback) {
  CHECK_OK(internal::ParseRecordIo(filename_, callback));
}

const TestRunOutput &OutputReceiver::model() {
  if (!output_.has_value()) output_ = ConsumeOutputOrDie();
  return *output_;
}

}  // namespace ocpdiag::results
