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

#ifndef OCPDIAG_LIB_RESULTS_INTERNAL_LOGGING_H_
#define OCPDIAG_LIB_RESULTS_INTERNAL_LOGGING_H_

#include <iostream>
#include <memory>
#include <string>

#include "google/protobuf/timestamp.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/util/type_resolver.h"
#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "ocpdiag/core/results/results.pb.h"
namespace ocpdiag {
namespace results {

constexpr char kInvalidRecordId[] = "invalid";

namespace internal {

// Returns the current time since epoch, or invalid time if unable to determine.
google::protobuf::Timestamp Now();

// Opens the requested file and returns the file-descriptor.
// Returns -1 (invalid descriptor) on empty string, or error if file is invalid
// or could not be opened.
absl::StatusOr<int> OpenAndGetDescriptor(const char* filepath);

// Returns a globally cached TypeResolver for the generated pool.
google::protobuf::util::TypeResolver* GeneratedResolver();

// Handles emission of OutputArtifacts for OCPDiag tests.
class ArtifactWriter {
 public:
  ArtifactWriter() = default;
  ArtifactWriter(int output_file_desc, bool also_print_to_stdout = false);
  // This ctor intended for use only in tests.
  ArtifactWriter(int output_file_desc, std::ostream* readable_out);

  // Write the artifact to output file/stream, if any.
  void Write(ocpdiag::results_pb::OutputArtifact&,
             google::protobuf::util::TypeResolver* resolver = GeneratedResolver());

  // Tells writer to flush the file buffer
  void Flush();

  // Checks whether a hardware_info_id is registered with the writer
  bool IsHwRegistered(absl::string_view i) const {
    if (proxy_ == nullptr) {
      return false;
    }
    return proxy_->registered_hw_.count(std::string(i));
  }

  // Checks whether a software_info_id is registered with the writer
  bool IsSwRegistered(absl::string_view i) const {
    if (proxy_ == nullptr) {
      return false;
    }
    return proxy_->registered_sw_.count(std::string(i));
  }

  // Internal use only. Adds the hardware_info_id to the set of registered infos
  void RegisterHwId(absl::string_view i) {
    if (proxy_ == nullptr || i == kInvalidRecordId) {
      return;
    }
    proxy_->registered_hw_.insert(std::string(i));
  }

  // Internal use only. Adds the software_info_id to the set of registered infos
  void RegisterSwId(absl::string_view i) {
    if (proxy_ == nullptr || i == kInvalidRecordId) {
      return;
    }
    proxy_->registered_sw_.insert(std::string(i));
  }

  // Internal use only.
  // Closes this writer only. All other writers must be closed
  // before file can be read. Should only be needed for unit tests
  void Close() {
    if (proxy_ == nullptr) {
      return;
    }
    proxy_.reset();  // releases this ptr only
  }

 private:
  // Thread-compatible struct that handles writing to shared streams/files.
  // Also stores and handles Hardware/SoftwareInfo registration records.
  struct WriterProxy {
    WriterProxy(int fd, std::ostream* readable = nullptr);
    ~WriterProxy();

    absl::flat_hash_set<std::string> registered_hw_;
    absl::flat_hash_set<std::string> registered_sw_;

    absl::Mutex mutex_;
    int sequence_num_ ABSL_GUARDED_BY(mutex_);
    std::ostream* readable_out_ ABSL_GUARDED_BY(mutex_);
  };

  std::shared_ptr<WriterProxy> proxy_;
};

// For writing Log artifacts
class LoggerInterface {
 private:
  virtual void WriteLog(ocpdiag::results_pb::Log::Severity,
                        absl::string_view msg) = 0;

 public:
  virtual ~LoggerInterface() = default;

  virtual void LogDebug(absl::string_view msg) = 0;
  virtual void LogInfo(absl::string_view msg) = 0;
  virtual void LogWarn(absl::string_view msg) = 0;
  virtual void LogError(absl::string_view msg) = 0;
  virtual void LogFatal(absl::string_view msg) = 0;
};

}  // namespace internal
}  // namespace results
}  // namespace ocpdiag

#endif  // OCPDIAG_LIB_RESULTS_INTERNAL_LOGGING_H_
