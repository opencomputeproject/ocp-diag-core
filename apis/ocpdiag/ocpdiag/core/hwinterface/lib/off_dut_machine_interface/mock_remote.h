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

#ifndef OCPDIAG_CORE_HWINTERFACE_LIB_OFF_DUT_MACHINE_INTERFACE_MOCK_REMOTE_H_
#define OCPDIAG_CORE_HWINTERFACE_LIB_OFF_DUT_MACHINE_INTERFACE_MOCK_REMOTE_H_

#include "gmock/gmock.h"
#include "ocpdiag/core/hwinterface/lib/off_dut_machine_interface/remote.h"

namespace ocpdiag::hwinterface::remote {

class MockConnInterface : public ConnInterface {
 public:
  MOCK_METHOD(absl::StatusOr<absl::Cord>, ReadFile,
              (absl::string_view file_name), (override));

  MOCK_METHOD(absl::Status, WriteFile,
              (absl::string_view file_name, absl::string_view data),
              (override));

  MOCK_METHOD(absl::StatusOr<CommandResult>, RunCommand,
              (absl::Duration timeout,
               const absl::Span<const absl::string_view> args,
               const CommandOption& options),
              (override));
};

}  // namespace ocpdiag::hwinterface::remote

#endif  // OCPDIAG_CORE_HWINTERFACE_LIB_OFF_DUT_MACHINE_INTERFACE_MOCK_REMOTE_H_
