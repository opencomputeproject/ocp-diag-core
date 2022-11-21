// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/lib/off_dut_machine_interface/remote_factory.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/status/status.h"
#include "ocpdiag/core/lib/off_dut_machine_interface/remote.h"
#include "ocpdiag/core/lib/off_dut_machine_interface/ssh/remote.h"
#include "ocpdiag/core/testing/status_matchers.h"

namespace ocpdiag::hwinterface {
namespace remote {

using ::ocpdiag::testing::StatusIs;

TEST(RemoteFactoryTest, RejectEmptyAddress) {
  EXPECT_THAT(NewConn(NodeSpec{""}),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(RemoteFactoryTest, RejectInvalidType) {
  absl::SetFlag(&FLAGS_mi_connection_type, "Telepathy");
  EXPECT_THAT(NewConn(NodeSpec{"dut"}),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(RemoteFactoryTest, SSHConnectionTypeSucceeds) {
  absl::SetFlag(&FLAGS_mi_connection_type, "SSH");
  EXPECT_OK(NewConn(NodeSpec{"dut"}));
}

TEST(GetSSHPathTest, SSHConnectionTypeSucceeds) {
  absl::string_view invalid_env = "/invalid/path:/another/invalid";
  auto ssh_path = GetSSHPath(invalid_env);
  EXPECT_THAT(ssh_path.status(), StatusIs(absl::StatusCode::kNotFound));
}

}  // namespace remote
}  // namespace ocpdiag::hwinterface
