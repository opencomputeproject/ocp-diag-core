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
