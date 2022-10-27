// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/recordio_iterator.h"

#include <fcntl.h>

#include <filesystem>  //

#include "gtest/gtest.h"
#include "ocpdiag/core/results/results.pb.h"
#include "riegeli/bytes/fd_writer.h"
#include "riegeli/records/record_writer.h"

namespace ocpdiag::results {
namespace {

using ::ocpdiag::results_pb::OutputArtifact;

class RecordIoContainerTest : public ::testing::Test {
 protected:
  RecordIoContainerTest()
      : filepath_(std::string(std::getenv("TEST_TMPDIR")) +
                  "/recordio_test_file") {
    if (std::filesystem::exists(filepath_))
      CHECK(std::filesystem::remove(filepath_)) << "Cannot remove temp file";

    int fd = open(filepath_.data(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    CHECK_NE(fd, -1);

    // Write a fixed set of protos for every test so we can iterate over them.
    OutputArtifact artifact;
    riegeli::RecordWriter writer(riegeli::FdWriter<>{fd});
    for (int i = 0; i < num_protos_; ++i)
      CHECK(writer.WriteRecord(artifact)) << writer.status().message();
    writer.Close();
  }

  const int num_protos_ = 10;
  std::string filepath_;
};

TEST_F(RecordIoContainerTest, EndIterator) {
  // Test iterating through the container using begin/end style syntax. This
  // verifies that our iterators will work in a range-based for loop.
  int cnt = 0;
  RecordIoIterator<OutputArtifact> iter(filepath_);
  RecordIoIterator<OutputArtifact> end;
  for (; iter != end; ++iter) {
    // Make sure the * operator works.
    OutputArtifact unused_artifact = std::move(*iter);
    cnt++;
  }
  EXPECT_EQ(cnt, num_protos_);
}

TEST_F(RecordIoContainerTest, BooleanOperator) {
  // The boolean operator is important for the pybind wrapper, because the
  // container's end iterator is not available so this is how we check if the
  // iterator is valid.
  int cnt = 0;
  for (auto iter = RecordIoIterator<OutputArtifact>(filepath_); iter; ++iter)
    cnt++;
  EXPECT_EQ(cnt, num_protos_);
}

TEST_F(RecordIoContainerTest, RangeBasedFor) {
  // The whole purpose of this iterator is to allow us to use range-based for
  // loops to iterate over recordio files, so this test makes sure we can do
  // that.
  struct MyContainer {
    std::string filepath;

    RecordIoIterator<OutputArtifact> begin() {
      return RecordIoIterator<OutputArtifact>(filepath);
    }
    RecordIoIterator<OutputArtifact> end() {
      return RecordIoIterator<OutputArtifact>();
    }
  } my_container{.filepath = filepath_};

  int cnt = 0;
  for (OutputArtifact artifact : my_container) cnt++;
  EXPECT_EQ(cnt, num_protos_);
}

}  // namespace
}  // namespace ocpdiag::results
