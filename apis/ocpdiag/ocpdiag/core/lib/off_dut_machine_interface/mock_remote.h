// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_HWINTERFACE_LIB_OFF_DUT_MACHINE_INTERFACE_MOCK_REMOTE_H_
#define OCPDIAG_CORE_HWINTERFACE_LIB_OFF_DUT_MACHINE_INTERFACE_MOCK_REMOTE_H_

#include "gmock/gmock.h"
#include "ocpdiag/core/lib/off_dut_machine_interface/remote.h"

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
