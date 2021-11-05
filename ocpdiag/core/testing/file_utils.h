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

#ifndef OCPDIAG_CORE_TESTING_FILE_UTILS_H_
#define OCPDIAG_CORE_TESTING_FILE_UTILS_H_

#include "google/protobuf/message.h"
#include "absl/strings/string_view.h"

namespace ocpdiag::testutils {

// Retrieves the full path of a test dependency file in the source tree.
std::string GetDataDependencyFilepath(absl::string_view file);

// Returns file contents from `file` in the source tree.
std::string GetDataDependencyFileContents(absl::string_view file);

// Writes proto in text format to the `file_full_path`.
void WriteProtoTextDebugFile(const google::protobuf::Message& msg,
                             const std::string& file_full_path);

}  // namespace ocpdiag::testutils

#endif  // OCPDIAG_CORE_TESTING_FILE_UTILS_H_
