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

#include "meltan/lib/params/utils.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "meltan/lib/params/testdata/test_params.pb.h"
#include "meltan/lib/testing/file_utils.h"
#include "meltan/lib/testing/status_matchers.h"

namespace {

using ::meltan::params::JsonFileToMessage;
using ::meltan::testing::StatusIs;
using ::meltan::lib::params::testdata::Params;

TEST(JsonFileToMessageTest, BadFile) {
  Params params;
  EXPECT_THAT(JsonFileToMessage("/not/a/valid/path", &params),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(JsonFileToMessageTest, BadMessage) {
  Params params;
  std::string test_path = meltan::testutils::GetDataDependencyFilepath(
      "meltan/lib/params/testdata/bad_test_params.json");
  EXPECT_THAT(JsonFileToMessage(test_path.c_str(), &params),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(JsonFileToMessageTest, GoodMessage) {
  Params params;
  std::string test_path = meltan::testutils::GetDataDependencyFilepath(
      "meltan/lib/params/testdata/test_params.json");
  ASSERT_OK(JsonFileToMessage(test_path.c_str(), &params));
  EXPECT_EQ(params.foo(), "foo_string");
}

}  // namespace
