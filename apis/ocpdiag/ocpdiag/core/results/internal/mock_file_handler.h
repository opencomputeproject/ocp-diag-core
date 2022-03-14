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

#ifndef OCPDIAG_CORE_RESULTS_INTERNAL_MOCK_FILE_HANDLER_H_
#define OCPDIAG_CORE_RESULTS_INTERNAL_MOCK_FILE_HANDLER_H_

#include <memory>
#include <sstream>

#include "gmock/gmock.h"
#include "absl/strings/string_view.h"
#include "ocpdiag/core/hwinterface/lib/off_dut_machine_interface/mock_remote.h"
#include "ocpdiag/core/hwinterface/lib/off_dut_machine_interface/remote.h"
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

  // Tells mock to execute code inside these methods, while keeping other method
  // mocks/stubs/expectations.
  // WARNING: Will override previously defined expectations on these methods.
  void DelegateToReal() {
    ON_CALL(*this, CopyRemoteFile(testing::_))
        .WillByDefault([&](ocpdiag::results_pb::File& f) {
          return this->FileHandler::CopyRemoteFile(f);
        });
    ON_CALL(*this, CopyLocalFile(testing::_, testing::_))
        .WillByDefault(
            [&](ocpdiag::results_pb::File& f, absl::string_view s) {
              return this->FileHandler::CopyLocalFile(f, s);
            });
  }
};

}  // namespace internal
}  // namespace results
}  // namespace ocpdiag

#endif  // OCPDIAG_CORE_RESULTS_INTERNAL_MOCK_FILE_HANDLER_H_
