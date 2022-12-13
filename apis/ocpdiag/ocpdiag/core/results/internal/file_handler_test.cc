// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/results/internal/file_handler.h"

#include <sys/stat.h>

#include <fstream>
#include <memory>
#include <sstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "ocpdiag/core/lib/off_dut_machine_interface/mock_remote.h"
#include "ocpdiag/core/lib/off_dut_machine_interface/remote.h"
#include "ocpdiag/core/results/internal/mock_file_handler.h"
#include "ocpdiag/core/testing/parse_text_proto.h"
#include "ocpdiag/core/testing/proto_matchers.h"
#include "ocpdiag/core/testing/status_matchers.h"

namespace {

using ::ocpdiag::hwinterface::remote::ConnInterface;
using ::ocpdiag::hwinterface::remote::MockConnInterface;
using ::ocpdiag::results::internal::FileHandler;
using ::ocpdiag::results::internal::MockFileHandler;
using ::ocpdiag::testing::EqualsProto;
using ::ocpdiag::testing::ParseTextProtoOrDie;
using ::ocpdiag::testing::Partially;
using ::ocpdiag::testing::StatusIs;
using ::testing::_;
using ::testing::Return;
using ::testing::StartsWith;

namespace rpb = ocpdiag::results_pb;

class FileHandlerTest : public ::testing::Test {
 public:
  FileHandlerTest() {
    ::testing::DefaultValue<
        absl::StatusOr<std::unique_ptr<ConnInterface>>>::SetFactory([] {
      return absl::StatusOr<std::unique_ptr<ConnInterface>>(
          std::make_unique<MockConnInterface>());
    });
    ::testing::DefaultValue<
        absl::StatusOr<std::unique_ptr<std::ostream>>>::SetFactory([] {
      return absl::StatusOr<std::unique_ptr<std::ostream>>(
          std::make_unique<std::stringstream>());
    });

    // Tells mock to execute the real file handler code inside these methods,
    // while keeping other method mocks/stubs/expectations.
    ON_CALL(mock_file_handler_, CopyRemoteFile(_))
        .WillByDefault([&](ocpdiag::results_pb::File& f) {
          return mock_file_handler_.FileHandler::CopyRemoteFile(f);
        });
    ON_CALL(mock_file_handler_, CopyLocalFile(_, _))
        .WillByDefault(
            [&](ocpdiag::results_pb::File& f, absl::string_view s) {
              return mock_file_handler_.FileHandler::CopyLocalFile(f, s);
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
  // Copy a file from one host, and a file with the same path on another host
  // and make sure they both get staged correctly.
  auto mock_conn1 =
      std::make_unique<ocpdiag::hwinterface::remote::MockConnInterface>();
  ON_CALL(*mock_conn1, ReadFile(_))
      .WillByDefault(Return(absl::Cord("content")));
  auto mock_conn2 =
      std::make_unique<ocpdiag::hwinterface::remote::MockConnInterface>();
  ON_CALL(*mock_conn2, ReadFile(_))
      .WillByDefault(Return(absl::Cord("content")));

  EXPECT_CALL(mock_file_handler_, GetConnInterface(_))
      .WillOnce([&](const std::string& s) { return std::move(mock_conn1); })
      .WillOnce([&](const std::string& s) { return std::move(mock_conn2); });

  const std::string common_path = "/tmp/data/file";

  rpb::File file1;
  file1.set_output_path(common_path);
  file1.set_node_address("node_address");
  ASSERT_OK(mock_file_handler_.CopyRemoteFile(file1));

  rpb::File file2;
  file2.set_output_path(common_path);
  file2.set_node_address("node_address2");
  ASSERT_OK(mock_file_handler_.CopyRemoteFile(file2));

  EXPECT_THAT(file1, Partially(EqualsProto(R"pb(
                upload_as_name: "node_address._tmp_data_file"
                node_address: "node_address"
              )pb")));
  EXPECT_THAT(file1.output_path(), StartsWith("file_"));

  EXPECT_THAT(file2, Partially(EqualsProto(R"pb(
                upload_as_name: "node_address2._tmp_data_file"
                node_address: "node_address2"
              )pb")));
  EXPECT_THAT(file2.output_path(), StartsWith("file_"));

  // Ensure that files from equivalent paths on different hosts have unique
  // local filenames.
  EXPECT_STRNE(file1.output_path().c_str(), file2.output_path().c_str());
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
      std::make_unique<ocpdiag::hwinterface::remote::MockConnInterface>();
  EXPECT_CALL(*mock_conn, ReadFile(_)).WillOnce(Return(absl::Cord("content")));

  // Stub GetConnInterface
  ON_CALL(mock_file_handler_, GetConnInterface(_))
      .WillByDefault(
          [&](const std::string& s) { return std::move(mock_conn); });

  ASSERT_OK(mock_file_handler_.CopyRemoteFile(file));

  EXPECT_THAT(file, Partially(EqualsProto(R"pb(
                upload_as_name: "upload_name.log"
                node_address: "node_address"
              )pb")));
  EXPECT_THAT(file.output_path(), StartsWith("file_"));
}

TEST_F(FileHandlerTest, NodeReadFileFail) {
  // Set expectations on mock node connection.
  auto mock_conn =
      std::make_unique<ocpdiag::hwinterface::remote::MockConnInterface>();
  EXPECT_CALL(*mock_conn, ReadFile(_))
      .WillOnce(Return(absl::InternalError("")));

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

  ASSERT_OK(mock_file_handler_.CopyLocalFile(file, tmpdir));
  // Make sure destination file is different
  ASSERT_STRNE(file.output_path().c_str(), tmp_filepath.c_str());
  std::ifstream dest(file.output_path());
  std::string got;
  std::getline(dest, got);
  ASSERT_STREQ(got.c_str(), "content");
}

TEST_F(FileHandlerTest, CopyLocalFilesWithSameName) {
  // Create two files with the same name, but existing in separate paths.
  std::string tmpdir = std::getenv(
      "TEST_TMPDIR");  // NOTE: TEST_TMPDIR only set when running on Forge.
  mkdir(absl::StrCat(tmpdir, "/path1").data(), 0777);
  mkdir(absl::StrCat(tmpdir, "/path2").data(), 0777);
  std::string tmp_filepath1 = absl::StrCat(tmpdir, "/path1/outputfile");
  std::string tmp_filepath2 = absl::StrCat(tmpdir, "/path2/outputfile");
  std::ofstream().open(tmp_filepath1);
  std::ofstream().open(tmp_filepath2);

  rpb::File file1;
  file1.set_output_path(tmp_filepath1);
  ASSERT_OK(mock_file_handler_.CopyLocalFile(file1, tmpdir));

  rpb::File file2;
  file2.set_output_path(tmp_filepath2);
  ASSERT_OK(mock_file_handler_.CopyLocalFile(file2, tmpdir));

  // Make sure the files both have unique output paths, so they don't collide.
  ASSERT_STRNE(file1.output_path().c_str(), file2.output_path().c_str());
}

TEST_F(FileHandlerTest, CopyLocalFileFail) {
  rpb::File file;

  // Tests are not allowed to write to CWD on forge.
  ASSERT_THAT(mock_file_handler_.CopyLocalFile(
                  file, ocpdiag::results::internal::kWorkingDir),
              StatusIs(absl::StatusCode::kUnknown));
}

}  // namespace
