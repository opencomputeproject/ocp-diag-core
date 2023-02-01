// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/structs.h"

#include "gtest/gtest.h"

namespace ocpdiag::results {

TEST(TestTestRunInfoStruct, CreateTestRunInfoFromMainArgs) {
  int argc = 3;
  const char* argv[] = {"diagname", "--flag", "flag_value"};
  TestRunStart run_info =
      TestRunStart::FromMainArgs("fake_name", "fake_version", &argc, argv, "");
  EXPECT_EQ(run_info.command_line, "diagname --flag flag_value");
}

}  // namespace ocpdiag::results
