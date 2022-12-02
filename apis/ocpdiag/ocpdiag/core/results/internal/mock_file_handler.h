// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_RESULTS_INTERNAL_MOCK_FILE_HANDLER_H_
#define OCPDIAG_CORE_RESULTS_INTERNAL_MOCK_FILE_HANDLER_H_

#include <memory>
#include <sstream>

#include "gmock/gmock.h"
#include "absl/strings/string_view.h"
#include "ocpdiag/core/lib/off_dut_machine_interface/mock_remote.h"
#include "ocpdiag/core/lib/off_dut_machine_interface/remote.h"
#include "ocpdiag/core/results/internal/file_handler.h"

namespace ocpdiag {
namespace results {
namespace internal {

class MockFileHandler : public FileHandler {
 public:
  MockFileHandler() = default;

  MOCK_METHOD(
      absl::StatusOr<std::unique_ptr<hwinterface::remote::ConnInterface>>,
      GetConnInterface, (const std::string&), (override));

  MOCK_METHOD(absl::StatusOr<std::unique_ptr<std::ostream>>, OpenLocalFile,
              (absl::string_view), (override));

  MOCK_METHOD(absl::Status, CopyRemoteFile,
              (ocpdiag::results_pb::File&), (override));

  MOCK_METHOD(absl::Status, CopyLocalFile,
              (ocpdiag::results_pb::File&, absl::string_view),
              (override));
};

}  // namespace internal
}  // namespace results
}  // namespace ocpdiag

#endif  // OCPDIAG_CORE_RESULTS_INTERNAL_MOCK_FILE_HANDLER_H_
