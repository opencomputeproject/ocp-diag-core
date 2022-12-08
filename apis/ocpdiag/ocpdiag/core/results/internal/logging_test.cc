// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/internal/logging.h"

#include <stdio.h>
#include <unistd.h>

#include <array>
#include <future>  //
#include <sstream>
#include <string>
#include <vector>

#include "google/protobuf/repeated_field.h"
#include "google/protobuf/util/json_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "ocpdiag/core/compat/status_converters.h"
#include "ocpdiag/core/results/internal/test_utils.h"
#include "ocpdiag/core/results/results.pb.h"
#include "ocpdiag/core/testing/proto_matchers.h"
#include "ocpdiag/core/testing/status_matchers.h"
#include "riegeli/bytes/fd_reader.h"
#include "riegeli/records/record_reader.h"

namespace ocpdiag {
namespace results {

using internal::ArtifactWriter;
using internal::TestFile;
using ::ocpdiag::testing::IsOkAndHolds;
using ::ocpdiag::testing::Partially;
using ::ocpdiag::testing::StatusIs;

namespace {

namespace rpb = ::ocpdiag::results_pb;

TEST(OpenAndReturnDescriptor, ValidFilepath) {
  EXPECT_OK(internal::OpenAndGetDescriptor("/dev/null"));
}

TEST(OpenAndReturnDescriptor, InvalidFilepath) {
  EXPECT_THAT(internal::OpenAndGetDescriptor("invalid/"),
              StatusIs(absl::StatusCode::kInternal));
}

TEST(OpenAndReturnDescriptor, EmptyString) {
  EXPECT_THAT(internal::OpenAndGetDescriptor(""), IsOkAndHolds(-1));
}

// Manually confirms that artifact contents are as expected
TEST(ArtifactWriter, Simple) {
  TestFile file;
  std::stringstream json_stream;
  {
    ArtifactWriter writer(dup(fileno(file.ptr)), &json_stream);
    rpb::OutputArtifact out_pb;
    rpb::TestStepArtifact* step_pb = out_pb.mutable_test_step_artifact();
    rpb::Diagnosis* diag = step_pb->mutable_diagnosis();
    *diag->mutable_hardware_info_id()->Add() = "7";
    diag->set_msg("test message");
    diag->set_symptom("test symptom");
    diag->set_type(rpb::Diagnosis::PASS);
    step_pb->set_test_step_id("7");
    writer.Write(out_pb);
  }  // writers fall out of scope and Flush/close file

  // Convert human-readable JSON output back to message
  rpb::OutputArtifact got;
  auto status =
      AsAbslStatus(google::protobuf::util::JsonStringToMessage(json_stream.str(), &got));
  ASSERT_OK(status);

  // Compare to what we expect (ignoring timestamp)
  auto want = R"pb(
    test_step_artifact {
      diagnosis {
        symptom: "test symptom"
        type: PASS
        msg: "test message"
        hardware_info_id: "7"
      }
      test_step_id: "7"
    }
    sequence_number: 0
  )pb";
  EXPECT_THAT(got, Partially(testing::EqualsProto(want)));

  // Read from results file
  riegeli::RecordReader reader(
      riegeli::FdReader(fileno(file.ptr), riegeli::FdReaderBase::Options()
                                              // Seek to beginning
                                              .set_independent_pos(0)));
  rpb::OutputArtifact file_got;
  if (!reader.ReadRecord(file_got)) FAIL() << "couldn't read";
  EXPECT_THAT(file_got, Partially(testing::EqualsProto(want)));
  reader.Close();
}

// Confirms that all newline '\n' characters are escaped '\\n'.
// And that all escaped newline '\\n' characters are ignored.
TEST(ArtifactWriter, replace_newlines) {
  const std::string input = "escape this newline\n, but not this one\\n";
  const std::string want = "escape this newline\\n, but not this one\\n";
  std::stringstream json_stream;
  {
    ArtifactWriter writer(-1, &json_stream);
    rpb::OutputArtifact out_pb;
    out_pb.mutable_test_step_artifact()->mutable_log()->set_text(input);
    writer.Write(out_pb);
  }  // writers fall out of scope and Flush/close file

  rpb::OutputArtifact got;
  auto status =
      AsAbslStatus(google::protobuf::util::JsonStringToMessage(json_stream.str(), &got));
  ASSERT_OK(status);

  ASSERT_STREQ(got.test_step_artifact().log().text().c_str(), want.c_str());
}

// Confirms that artifact integrity is maintained with many asynchronous writes
TEST(ArtifactWriter, ThreadSafetyCheck) {
  TestFile file;
  const int writer_copies = 20;
  const int artifact_count = 1000;
  {
    ArtifactWriter writer(dup(fileno(file.ptr)), nullptr);
    std::vector<std::future<void>> threads;
    for (int i = 0; i < writer_copies; i++) {
      threads.push_back(std::async(std::launch::async, [&] {
        for (int i = 0; i < artifact_count; i++) {
          rpb::OutputArtifact out_pb;
          writer.Write(out_pb);
        }
      }));
    }
    for (std::future<void>& thread : threads) thread.wait();
  }  // writer falls out of scope and Flush/close file

  // Read from results file
  riegeli::RecordReader reader(
      riegeli::FdReader(fileno(file.ptr), riegeli::FdReaderBase::Options()
                                              // Seek to beginning
                                              .set_independent_pos(0)));

  int want_count = writer_copies * artifact_count;
  int got_count = 0;
  rpb::OutputArtifact got;
  while (reader.ReadRecord(got)) got_count++;
  EXPECT_EQ(got_count, want_count);
}

// Confirms that writer does not write after Close() and fails gracefully
TEST(ArtifactWriter, WriteFailAfterClose) {
  TestFile file;
  std::stringstream json;
  ArtifactWriter writer(fileno(file.ptr), &json);
  writer.Close();
  rpb::OutputArtifact out_pb;
  *out_pb.mutable_test_step_artifact() = rpb::TestStepArtifact();
  writer.Write(out_pb);

  // Expect empty outputs
  riegeli::RecordReader reader(
      riegeli::FdReader(fileno(file.ptr), riegeli::FdReaderBase::Options()
                                              // Seek to beginning
                                              .set_independent_pos(0)));
  while (reader.ReadRecord(out_pb)) {
    FAIL() << "unexpected record found in file: " << out_pb.DebugString();
  }
  EXPECT_TRUE(json.str().empty()) << "unexpected contents: " << json.str();
}

}  // namespace

}  // namespace results
}  // namespace ocpdiag
