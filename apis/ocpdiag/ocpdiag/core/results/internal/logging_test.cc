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
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "ocpdiag/core/compat/status_converters.h"
#include "ocpdiag/core/results/internal/test_utils.h"
#include "ocpdiag/core/results/results.pb.h"
#include "ocpdiag/core/testing/proto_matchers.h"
#include "ocpdiag/core/testing/status_matchers.h"

namespace ocpdiag {
namespace results {

using internal::ArtifactWriter;
using internal::TestFile;
using ::ocpdiag::testing::EqualsProto;
using ::ocpdiag::testing::IsOkAndHolds;
using ::ocpdiag::testing::Partially;
using ::ocpdiag::testing::StatusIs;
using ::testing::Pointwise;

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
}

//
TEST(ArtifactWriter, MultipleWritersInOrder) {
  std::vector<rpb::OutputArtifact> wants;
  std::vector<rpb::OutputArtifact> gots;
  TestFile file;
  std::stringstream json_stream;
  {
    ArtifactWriter writer1(dup(fileno(file.ptr)), &json_stream);
    ArtifactWriter writer2 = writer1;
    rpb::OutputArtifact out_pb;
    rpb::TestStepArtifact* step_pb = out_pb.mutable_test_step_artifact();
    rpb::Log* log_pb = step_pb->mutable_log();
    step_pb->set_test_step_id("76");
    log_pb->set_text("Hello, ");
    writer1.Write(out_pb);
    wants.push_back(out_pb);

    log_pb->set_text("World!");
    writer2.Write(out_pb);
    wants.push_back(out_pb);
  }  // writers fall out of scope and Flush/close file
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
    ArtifactWriter root_writer(dup(fileno(file.ptr)), nullptr);
    std::array<ArtifactWriter, writer_copies> writers;
    for (int i = 0; i < writer_copies; i++) {
      writers[i] = root_writer;
    }
    std::vector<std::future<void>> threads;
    for (auto& writer : writers) {
      threads.push_back(std::async(std::launch::async, [&] {
        for (int i = 0; i < artifact_count; i++) {
          rpb::OutputArtifact out_pb;
          writer.Write(out_pb);
        }
      }));
    }
    for (std::future<void>& thread : threads) {
      thread.wait();
    }
  }  // writers fall out of scope and Flush/close file
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
}

}  // namespace

}  // namespace results
}  // namespace ocpdiag
