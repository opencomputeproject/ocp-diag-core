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
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/util/json_util.h"
#include "google/protobuf/util/type_resolver_util.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_replace.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "ocpdiag/core/compat/status_converters.h"
#include "ocpdiag/core/params/utils.h"
#include "ocpdiag/core/results/results.pb.h"

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

absl::StatusOr<int> OpenAndGetDescriptor(const char* filepath) {
  int fd = -1;
  if (strlen(filepath) != 0) {
    fd = open(filepath, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
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
}

void ArtifactWriter::Flush() {
}

ArtifactWriter::WriterProxy::WriterProxy(int fd, std::ostream* readable)
    : sequence_num_(0), readable_out_(readable) {
}

ArtifactWriter::WriterProxy::~WriterProxy() {
}

}  // namespace internal
}  // namespace results
}  // namespace ocpdiag
