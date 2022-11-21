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

class RecordIoIteratorTest : public ::testing::Test {
 protected:
  RecordIoIteratorTest()
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

TEST_F(RecordIoIteratorTest, EndIterator) {
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

TEST_F(RecordIoIteratorTest, BooleanOperator) {
  // The boolean operator is important for the pybind wrapper, because the
  // container's end iterator is not available so this is how we check if the
  // iterator is valid.
  int cnt = 0;
  for (auto iter = RecordIoIterator<OutputArtifact>(filepath_); iter; ++iter)
    cnt++;
  EXPECT_EQ(cnt, num_protos_);
}

TEST_F(RecordIoIteratorTest, RangeBasedForContainer) {
  int cnt = 0;
  RecordIoContainer<OutputArtifact> container(filepath_);
  ASSERT_EQ(container.file_path(), filepath_);

  for (OutputArtifact artifact : container) cnt++;
  EXPECT_EQ(cnt, num_protos_);
}

}  // namespace
}  // namespace ocpdiag::results
