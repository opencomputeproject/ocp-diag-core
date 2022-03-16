// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/params/utils.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "ocpdiag/core/params/testdata/test_params.pb.h"
#include "ocpdiag/core/testing/file_utils.h"
#include "ocpdiag/core/testing/status_matchers.h"

namespace {

using ::ocpdiag::params::JsonFileToMessage;
using ::ocpdiag::testing::StatusIs;
using ::ocpdiag::core::params::testdata::Params;

TEST(JsonFileToMessageTest, BadFile) {
  Params params;
  EXPECT_THAT(JsonFileToMessage("/not/a/valid/path", &params),
              StatusIs(absl::StatusCode::kNotFound));
}

TEST(JsonFileToMessageTest, BadMessage) {
  Params params;
  std::string test_path = ocpdiag::testutils::GetDataDependencyFilepath(
      "ocpdiag/core/params/testdata/bad_test_params.json");
  EXPECT_THAT(JsonFileToMessage(test_path.c_str(), &params),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(JsonFileToMessageTest, GoodMessage) {
  Params params;
  std::string test_path = ocpdiag::testutils::GetDataDependencyFilepath(
      "ocpdiag/core/params/testdata/test_params.json");
  ASSERT_OK(JsonFileToMessage(test_path.c_str(), &params));
  EXPECT_EQ(params.foo(), "foo_string");
}

}  // namespace
