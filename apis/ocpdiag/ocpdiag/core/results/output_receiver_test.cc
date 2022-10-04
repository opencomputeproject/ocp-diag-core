// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/output_receiver.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "ocpdiag/core/compat/status_macros.h"
#include "ocpdiag/core/results/results.h"
#include "ocpdiag/core/results/results.pb.h"
#include "ocpdiag/core/testing/status_matchers.h"

namespace ocpdiag::results {
namespace {

TEST(OutputReceiverTest, Nominal) {
  OutputReceiver output;
  {
    ASSERT_OK_AND_ASSIGN(auto test_run,
                         ResultApi().InitializeTestRun("MyTest"));
    test_run->StartAndRegisterInfos({});
    test_run->LogInfo("Test log");
  }

  // Check that the output model is populated.
  EXPECT_TRUE(output.model().start.has_value());
  EXPECT_TRUE(output.model().end.has_value());

  // Iterate through the raw results, stopping after 2 records to test that we
  // can stop.
  int count = 0;
  output.Iterate([&](ocpdiag::results_pb::OutputArtifact artifact) {
    count++;
    return count != 2;
  });
  EXPECT_EQ(count, 2);
}

}  // namespace
}  // namespace ocpdiag::results
