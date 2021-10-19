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

#include "meltan/lib/params/fake_params.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "meltan/lib/params/testdata/ambiguous_params.pb.h"
#include "meltan/lib/params/testdata/test_params.pb.h"
#include "meltan/lib/params/utils.h"
#include "meltan/lib/testing/proto_matchers.h"
#include "meltan/lib/testing/status_matchers.h"

using ::meltan::testing::EqualsProto;
using ::testing::Ne;

namespace meltan {
namespace params {
namespace {

TEST(FakeParamsTest, ValidParamsRoundTrip) {
  meltan::lib::params::testdata::Params faked;
  faked.set_foo("fake_foo");
  faked.set_bar(4321);
  faked.add_subs()->add_other(6789);
  auto cleanup = FakeParams(faked);
  meltan::lib::params::testdata::Params gotten;
  ASSERT_OK(GetParams(&gotten));
  EXPECT_THAT(gotten, EqualsProto(faked));
}

TEST(FakeParamsTest, InvalidParamsError) {
  meltan::lib::params::testdata::Params faked;
  faked.set_foo("fake_foo");
  faked.set_bar(4321);
  faked.add_subs()->add_other(6789);
  auto cleanup = FakeParams(faked);
  meltan::lib::params::testdata::ParamsMessage gotten;
  EXPECT_THAT(GetParams(&gotten), Ne(absl::OkStatus()));
}

}  // namespace
}  // namespace params
}  // namespace meltan
