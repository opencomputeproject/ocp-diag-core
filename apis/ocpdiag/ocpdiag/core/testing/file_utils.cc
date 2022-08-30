// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/testing/file_utils.h"

#include <fstream>
#include <sstream>

#include "google/protobuf/text_format.h"
#include "absl/strings/str_cat.h"

namespace ocpdiag::testutils {

constexpr const char* kSrcTestDir = "TEST_SRCDIR";
constexpr const char* kSrcWorkspace = "TEST_WORKSPACE";
constexpr absl::string_view kGoogleWorkspace = "google3";

std::string GetDataDependencyFilepath(absl::string_view file) {
  const absl::string_view source_dir = getenv(kSrcTestDir);
  const absl::string_view workspace = getenv(kSrcWorkspace);
  // If using bazel, the workspace name must be appended.
  if (workspace != kGoogleWorkspace) {
    return absl::StrCat(source_dir, "/", workspace, "/", file);
  }
  return absl::StrCat(source_dir, "/", file);
}

std::string GetDataDependencyFileContents(absl::string_view file) {
  std::ifstream reader(GetDataDependencyFilepath(file));
  std::ostringstream out;
  out << reader.rdbuf();
  return out.str();
}

void WriteProtoTextDebugFile(const google::protobuf::Message& msg,
                             const std::string& file_full_path) {
  std::ofstream file(file_full_path);
  std::string out;
  google::protobuf::TextFormat::PrintToString(msg, &out);
  file << out;
  file.close();
}

}  // namespace ocpdiag::testutils
