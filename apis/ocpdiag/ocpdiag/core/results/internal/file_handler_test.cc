// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/internal/file_handler.h"

#include <fstream>
#include <memory>
#include <sstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "ocpdiag/core/hwinterface/lib/off_dut_machine_interface/mock_remote.h"
#include "ocpdiag/core/hwinterface/lib/off_dut_machine_interface/remote.h"
#include "ocpdiag/core/results/internal/mock_file_handler.h"
#include "ocpdiag/core/testing/parse_text_proto.h"
#include "ocpdiag/core/testing/proto_matchers.h"
#include "ocpdiag/core/testing/status_matchers.h"

namespace {

using ::ocpdiag::hwinterface::remote::ConnInterface;
using ::ocpdiag::hwinterface::remote::MockConnInterface;
using ::ocpdiag::results::internal::FileHandler;
using ::ocpdiag::results::internal::MockFileHandler;
using ::ocpdiag::testing::ParseTextProtoOrDie;
using ::ocpdiag::testing::StatusIs;
using ::testing::_;

namespace rpb = ocpdiag::results_pb;

class FileHandlerTest : public ::testing::Test {
 public:
  FileHandlerTest() {
    ::testing::DefaultValue<absl::StatusOr<
        std::unique_ptr<ConnInterface>>>::SetFactory([] {
      return absl::StatusOr<
          std::unique_ptr<ConnInterface>>(
          std::make_unique<MockConnInterface>());
    });
    ::testing::DefaultValue<
        absl::StatusOr<std::unique_ptr<std::ostream>>>::SetFactory([] {
      return absl::StatusOr<std::unique_ptr<std::ostream>>(
          std::make_unique<std::stringstream>());
    });
  }

  FileHandler file_handler_;
  MockFileHandler mock_file_handler_;
};

TEST_F(FileHandlerTest, GetConnInterfaceFail) {
  const std::string empty_node_addr = "";
  EXPECT_THAT(file_handler_.GetConnInterface(empty_node_addr),
              StatusIs(absl::StatusCode::kUnavailable));
}

TEST_F(FileHandlerTest, OpenLocalFileFail) {
  const std::string invalid_filename = "/";
  EXPECT_THAT(file_handler_.OpenLocalFile(invalid_filename),
              StatusIs(absl::StatusCode::kUnknown));
}

TEST_F(FileHandlerTest, CopyRemoteFile) {
  rpb::File file;
  file.set_output_path("/tmp/data/file");
  file.set_node_address("node_address");

  // Set expectations on mock node connection.
  auto mock_conn =
      absl::make_unique<ocpdiag::hwinterface::remote::MockConnInterface>();
  EXPECT_CALL(*mock_conn, ReadFile(_))
      .WillOnce(::testing::Return(absl::Cord("content")));

  // We want the mock to actually execute `CopyRemoteFile`, instead of just
  // return a default value. Other methods are mocked by default.
  mock_file_handler_.DelegateToReal();

  // Stub GetConnInterface
  ON_CALL(mock_file_handler_, GetConnInterface(_))
      .WillByDefault(
          [&](const std::string& s) { return std::move(mock_conn); });

  ASSERT_OK(mock_file_handler_.CopyRemoteFile(file));

  rpb::File want = ParseTextProtoOrDie(absl::StrFormat(
      R"pb(
        upload_as_name: "node_address._tmp_data_file"
        output_path: "node_address._tmp_data_file"
        node_address: "node_address"
      )pb"));
  EXPECT_THAT(file, ocpdiag::testing::EqualsProto(want));
}

// Copy remote file, but with custom upload name, expect name to stay the same.
TEST_F(FileHandlerTest, CopyRemoteFileWithUploadName) {
  rpb::File file;
  file.set_output_path("/tmp/data/file");
  file.set_node_address("node_address");
  // set custom upload name, expect it to stay the same.
  file.set_upload_as_name("upload_name.log");

  // Set expectations on mock node connection.
  auto mock_conn =
      absl::make_unique<ocpdiag::hwinterface::remote::MockConnInterface>();
  EXPECT_CALL(*mock_conn, ReadFile(_))
      .WillOnce(::testing::Return(absl::Cord("content")));

  // We want the mock to actually execute `CopyRemoteFile`, instead of just
  // return a default value. Other methods are mocked by default.
  mock_file_handler_.DelegateToReal();

  // Stub GetConnInterface
  ON_CALL(mock_file_handler_, GetConnInterface(_))
      .WillByDefault(
          [&](const std::string& s) { return std::move(mock_conn); });

  ASSERT_OK(mock_file_handler_.CopyRemoteFile(file));

  rpb::File want = ParseTextProtoOrDie(absl::StrFormat(
      R"pb(
        upload_as_name: "upload_name.log"
        output_path: "node_address._tmp_data_file"
        node_address: "node_address"
      )pb"));
  EXPECT_THAT(file, ocpdiag::testing::EqualsProto(want));
}

TEST_F(FileHandlerTest, NodeReadFileFail) {
  // Set expectations on mock node connection.
  auto mock_conn =
      absl::make_unique<ocpdiag::hwinterface::remote::MockConnInterface>();
  EXPECT_CALL(*mock_conn, ReadFile(_))
      .WillOnce(::testing::Return(absl::InternalError("")));

  // We want the mock to actually execute `CopyRemoteFile`, instead of just
  // return a default value. Other methods are mocked by default.
  mock_file_handler_.DelegateToReal();

  // Stub GetConnInterface
  ON_CALL(mock_file_handler_, GetConnInterface(_))
      .WillByDefault(
          [&](const std::string& s) { return std::move(mock_conn); });

  rpb::File file;
  EXPECT_THAT(mock_file_handler_.CopyRemoteFile(file),
              StatusIs(absl::StatusCode::kUnknown));
}

TEST_F(FileHandlerTest, CopyLocalFile) {
  // Create real local file in test dir.
  std::string tmpdir = std::getenv(
      "TEST_TMPDIR");  // NOTE: TEST_TMPDIR only set when running on Forge.
  std::string tmp_filepath = absl::StrCat(tmpdir, "/outputfile");
  std::ofstream src;
  src.open(tmp_filepath);
  src << "content";
  src.close();

  rpb::File file;
  file.set_output_path(tmp_filepath);

  // We want the mock to actually execute `CopyLocalFile`, instead of just
  // return a default value. Other methods are mocked by default.
  mock_file_handler_.DelegateToReal();

  ASSERT_OK(mock_file_handler_.CopyLocalFile(file, tmpdir));
  // Make sure destination file is different
  ASSERT_STRNE(file.output_path().c_str(), tmp_filepath.c_str());
  std::ifstream dest(file.output_path());
  std::string got;
  std::getline(dest, got);
  ASSERT_STREQ(got.c_str(), "content");
}

TEST_F(FileHandlerTest, CopyLocalFileFail) {
  rpb::File file;

  // We want the mock to actually execute `CopyLocalFile`, instead of just
  // return a default value. Other methods are mocked by default.
  mock_file_handler_.DelegateToReal();

  // Tests are not allowed to write to CWD on forge.
  ASSERT_THAT(mock_file_handler_.CopyLocalFile(
                  file, ocpdiag::results::internal::kWorkingDir),
              StatusIs(absl::StatusCode::kUnknown));
}

}  // namespace
