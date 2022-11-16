// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/validation/unit_test_validator.h"

#include <fcntl.h>
#include <stdio.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "ocpdiag/core/results/results.pb.h"
#include "ocpdiag/core/testing/file_utils.h"
#include "ocpdiag/core/testing/status_matchers.h"
#include "riegeli/bytes/fd_writer.h"
#include "riegeli/records/record_writer.h"

namespace ocpdiag::results {
namespace {

namespace rpb = ::ocpdiag::results_pb;

class UnitTestValidatorTest : public ::testing::TestWithParam<std::string> {
 protected:
  void WriteArtifact(const rpb::OutputArtifact &artifact) {
    ASSERT_TRUE(writer_.WriteRecord(artifact));
    writer_.Flush(riegeli::FlushType::kFromMachine);
  }

  std::string path_{testutils::MkTempFileOrDie("unit_test_validator")};
  riegeli::RecordWriter<riegeli::FdWriter<>> writer_{
      riegeli::FdWriter<>(open(path_.c_str(), O_WRONLY | O_CREAT))};
};

TEST_F(UnitTestValidatorTest, Call) {
  WriteArtifact(rpb::OutputArtifact{});
  EXPECT_OK(ValidateRecordIo(path_));
}

}  // namespace
}  // namespace ocpdiag::results
