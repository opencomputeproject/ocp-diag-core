// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/output_receiver.h"

#include "gtest/gtest.h"
#include "ocpdiag/core/results/results.h"
#include "ocpdiag/core/results/results.pb.h"

namespace ocpdiag::results {
namespace {

TEST(OutputReceiverTest, Nominal) {
  OutputReceiver receiver;
  {
    TestRun test_run("Test", receiver.artifact_writer());
    test_run.StartAndRegisterInfos({});
    test_run.LogInfo("Test log");
  }

  // Check that the output model is populated.
  EXPECT_TRUE(receiver.model().start.has_value());
  EXPECT_TRUE(receiver.model().end.has_value());

  // Iterate through the raw results.
  int count = 0;
  for (auto unused_artifact : receiver) count++;
  EXPECT_EQ(count, 3);
}

}  // namespace
}  // namespace ocpdiag::results
