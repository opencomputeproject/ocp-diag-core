// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/params/fake_params.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "ocpdiag/core/params/testdata/ambiguous_params.pb.h"
#include "ocpdiag/core/params/testdata/test_params.pb.h"
#include "ocpdiag/core/params/utils.h"
#include "ocpdiag/core/testing/proto_matchers.h"
#include "ocpdiag/core/testing/status_matchers.h"

using ::ocpdiag::testing::EqualsProto;
using ::testing::Ne;

namespace ocpdiag {
namespace params {
namespace {

TEST(FakeParamsTest, ValidParamsRoundTrip) {
  ocpdiag::core::params::testdata::Params faked;
  faked.set_foo("fake_foo");
  faked.set_bar(4321);
  faked.add_subs()->add_other(6789);
  auto cleanup = FakeParams(faked);
  ocpdiag::core::params::testdata::Params gotten;
  ASSERT_OK(GetParams(&gotten));
  EXPECT_THAT(gotten, EqualsProto(faked));
}

TEST(FakeParamsTest, InvalidParamsError) {
  ocpdiag::core::params::testdata::Params faked;
  faked.set_foo("fake_foo");
  faked.set_bar(4321);
  faked.add_subs()->add_other(6789);
  auto cleanup = FakeParams(faked);
  ocpdiag::core::params::testdata::ParamsMessage gotten;
  EXPECT_THAT(GetParams(&gotten), Ne(absl::OkStatus()));
}

}  // namespace
}  // namespace params
}  // namespace ocpdiag
