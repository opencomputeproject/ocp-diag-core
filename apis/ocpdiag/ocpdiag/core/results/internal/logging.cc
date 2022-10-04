// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/internal/logging.h"

#include <fcntl.h>
#include <string.h>

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "google/protobuf/timestamp.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/util/json_util.h"
#include "google/protobuf/util/type_resolver_util.h"
#include "absl/cleanup/cleanup.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_replace.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "ocpdiag/core/compat/status_converters.h"
#include "ocpdiag/core/results/results.pb.h"
#include "riegeli/bytes/fd_reader.h"
#include "riegeli/bytes/fd_writer.h"
#include "riegeli/records/record_reader.h"
#include "riegeli/records/records_metadata.pb.h"

namespace ocpdiag {
namespace results {
namespace internal {

namespace rpb = ocpdiag::results_pb;

constexpr absl::Duration kFlushFreq = absl::Minutes(1);

google::protobuf::Timestamp Now() {
  auto time = absl::Now();
  google::protobuf::Timestamp ret;
  auto sec = absl::ToUnixSeconds(time);
  auto ns = (time - absl::FromUnixSeconds(sec)) / absl::Nanoseconds(1);
  // sec must be [0001-01-01T00:00:00Z, 9999-12-31T23:59:59.999999999Z]
  if ((sec < -62135596800 || sec > 253402300799) ||
      (ns < 0 || ns > 999999999)) {
    std::cerr << "Failed to generate timestamp for MeasurementElement"
              << std::endl;
    sec = -1;
    ns = -1;
  }
  ret.set_seconds(sec);
  ret.set_nanos(ns);
  return ret;
}

absl::StatusOr<int> OpenAndGetDescriptor(absl::string_view filepath) {
  int fd = -1;
  if (!filepath.empty()) {
    fd = open(filepath.data(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1) {
      return absl::InternalError(absl::StrFormat(
          "Failed to open requested output file \"%s\"", filepath));
    }
  }
  return fd;
}

ArtifactWriter::ArtifactWriter(int output_file_desc, std::ostream* readable)
    : proxy_(new WriterProxy(output_file_desc, readable)) {}

ArtifactWriter::ArtifactWriter(int output_file_desc, bool also_print_to_stdout)
    : proxy_(new WriterProxy(output_file_desc, nullptr)) {}

const char* kTypeUrlPrefix = "type.googleapis.com";
std::string GetTypeUrl(const google::protobuf::Message& message) {
  return std::string(kTypeUrlPrefix) + "/" +
         message.GetDescriptor()->full_name();
}

google::protobuf::util::TypeResolver* GeneratedResolver() {
  static google::protobuf::util::TypeResolver* generated_resolver =
      google::protobuf::util::NewTypeResolverForDescriptorPool(
          kTypeUrlPrefix, google::protobuf::DescriptorPool::generated_pool());
  return generated_resolver;
}

void ArtifactWriter::Write(rpb::OutputArtifact& out_pb,
                           google::protobuf::util::TypeResolver* resolver) {
  if (proxy_ == nullptr) {
    return;
  }
  auto time = Now();
  out_pb.mutable_timestamp()->set_seconds(time.seconds());
  out_pb.mutable_timestamp()->set_nanos(time.nanos());

  absl::MutexLock lock(&proxy_->mutex_);
  out_pb.set_sequence_number(proxy_->sequence_num_++);
  // Output JSONL proto, if requested
  if (proxy_->readable_out_ != nullptr) {
    std::string json;
    google::protobuf::util::JsonPrintOptions opts;
    opts.always_print_primitive_fields = true;
#ifdef EXPAND_JSONL
    // pretty-prints the JSON output for easier debug
    opts.add_whitespace = true;
#endif

    const std::string binary_input = out_pb.SerializeAsString();
    google::protobuf::io::ArrayInputStream input_stream(binary_input.data(),
                                              binary_input.size());
    google::protobuf::io::StringOutputStream output_stream(&json);
    absl::Status status = AsAbslStatus(BinaryToJsonStream(
        resolver, GetTypeUrl(out_pb), &input_stream, &output_stream, opts));
    if (!status.ok()) {
      std::cerr << "Failed to serialize message: " << status.ToString()
                << std::endl;
      return;
    }

    // Escape all newline characters, otherwise parsers may fail.
    // Two steps here to avoid needing regex capture-groups.
    absl::StrReplaceAll({{R"(\\n)", R"(\n)"}}, &json);
    absl::StrReplaceAll({{R"(\n)", R"(\\n)"}}, &json);
    *proxy_->readable_out_ << json << std::endl;
  }

  // Output compressed proto to file
  if (proxy_->file_out_.ok()) {
    if (!proxy_->file_out_.WriteRecord(out_pb)) {
      std::cerr << "Failed to write proto record to file: "
                << "\"" << out_pb.DebugString() << "\"" << std::endl
                << "File writer error: "
                << proxy_->file_out_.status().ToString() << std::endl;
    }
  }
}

void ArtifactWriter::Flush() {
  if (proxy_) {
    proxy_->FlushFileBuffer();
  }
}

void ArtifactWriter::WriterProxy::FlushFileBuffer()
    ABSL_LOCKS_EXCLUDED(mutex_) {
  absl::MutexLock l(&mutex_);
  file_out_.Flush(riegeli::FlushType::kFromMachine);
}

ArtifactWriter::WriterProxy::WriterProxy(int fd, std::ostream* readable)
    : sequence_num_(0), readable_out_(readable) {
  if (fd < 0) {
    return;
  }
  // Set record metadata, so Riegeli file is self-describing
  riegeli::RecordsMetadata metadata;
  riegeli::SetRecordType(
      *ocpdiag::results_pb::OutputArtifact::GetDescriptor(),
      metadata);
  file_out_.Reset(
      riegeli::FdWriter(fd),
      riegeli::RecordWriterBase::Options().set_metadata(std::move(metadata)));
  if (!file_out_.ok()) {
    std::cerr << "File writer error: " << file_out_.status().ToString()
              << std::endl;
  }
}

ArtifactWriter::WriterProxy::~WriterProxy() {
  file_out_.Close();
}

absl::Status ParseRecordIo(absl::string_view filepath,
                           std::function<bool(rpb::OutputArtifact)> callback) {
  int fd = open(filepath.data(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    return absl::InternalError(absl::StrFormat(
        "Failed to open requested output file \"%s\"", filepath));
  }
  auto close_fd = absl::MakeCleanup([fd]() { close(fd); });

  riegeli::RecordReader reader(riegeli::FdReader(
      fd, riegeli::FdReaderBase::Options().set_independent_pos(0)));
  auto close_reader = absl::MakeCleanup([&reader]() { reader.Close(); });

  rpb::OutputArtifact artifact;
  while (reader.ReadRecord(artifact)) {
    if (!callback(std::move(artifact))) return absl::OkStatus();
  }
  return reader.status();
}

}  // namespace internal
}  // namespace results
}  // namespace ocpdiag
