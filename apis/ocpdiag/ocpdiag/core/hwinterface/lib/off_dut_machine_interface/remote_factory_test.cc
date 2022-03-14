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

#include "ocpdiag/core/hwinterface/lib/off_dut_machine_interface/remote_factory.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/status/status.h"
#include "ocpdiag/core/hwinterface/lib/off_dut_machine_interface/remote.h"
#include "ocpdiag/core/hwinterface/lib/off_dut_machine_interface/ssh/remote.h"
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

TEST(RemoteFactoryTest, DefaultConnectionTypeSucceeds) {
  absl::SetFlag(&FLAGS_mi_connection_type, "");
  EXPECT_OK(NewConn(NodeSpec{"dut"}));
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
