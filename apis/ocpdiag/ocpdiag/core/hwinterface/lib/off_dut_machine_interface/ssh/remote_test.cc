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

#include "ocpdiag/core/hwinterface/lib/off_dut_machine_interface/ssh/remote.h"

#include <csignal>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string_view>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "absl/types/span.h"
#include "ocpdiag/core/hwinterface/lib/off_dut_machine_interface/remote.h"
#include "ocpdiag/core/testing/status_matchers.h"

namespace ocpdiag::hwinterface {
namespace remote {

using ::ocpdiag::testing::StatusIs;
using ::testing::Eq;

// constexpr absl::string_view kSshPath = "/usr/bin/ssh";

// SshTest is for testing the SSHConnInterface. It creates an executable script
// named "ssh" in tmp folder. By modifying PATH, we can let execvp to execute
// the script as a fake ssh. Then we can put different command in the script to
// veirfy the SSHConnInterface behavior.
class SshTest : public ::testing::Test {
 public:
  SshTest() : SshTest("", "") {}
  SshTest(absl::string_view ssh_key_path,
          absl::string_view ssh_tunnel_file_path)
      : ssh_conn_(NodeSpec{"dut"}, ssh_key_path, ssh_tunnel_file_path,
                  ssh_path_.string()) {}

 protected:
  static void SetUpTestSuite() {
    // Update the path to prepend the temporary directory.
    std::filesystem::path tmpdir = std::filesystem::temp_directory_path();
    absl::string_view old_path = getenv("PATH");
    // PATH should be appended with colon.
    std::string new_path = absl::StrCat(tmpdir.native(), ":", old_path);
    setenv("PATH", new_path.c_str(), /*__replace=*/true);
    // Create an executable SSH script.
    ssh_path_ = tmpdir / "ssh";
    std::ofstream(ssh_path_, std::ios_base::out | std::ios_base::trunc)
        << "#!/bin/sh" << std::endl;
    std::filesystem::permissions(ssh_path_, std::filesystem::perms::all);
  }

  void SetUp() {
    // Reset SSH behavior on each test.
    std::ofstream(ssh_path_, std::ios_base::out | std::ios_base::trunc)
        << "#!/bin/sh" << std::endl;
  }

  void FakeSsh(absl::string_view script) {
    std::ofstream(ssh_path_, std::ios_base::out | std::ios_base::trunc)
        << "#!/bin/sh" << std::endl
        << script << std::endl;
  }

  absl::StatusOr<ConnInterface::CommandResult> RunCommand(
      absl::string_view command,
      ConnInterface::CommandOption options = ConnInterface::CommandOption()) {
    absl::Span<absl::string_view> commands = absl::MakeSpan(&command, 1);
    return ssh_conn_.RunCommand(absl::Seconds(1), commands, options);
  }

  absl::StatusOr<absl::Cord> ReadFile(absl::string_view file_name) {
    return ssh_conn_.ReadFile(file_name);
  }

  absl::Status WriteFile(absl::string_view file_name, absl::string_view data) {
    return ssh_conn_.WriteFile(file_name, data);
  }

  std::string ReadLocalFile(std::string_view file_name) {
    std::stringstream buffer;
    buffer << std::ifstream(std::string(file_name).c_str()).rdbuf();
    return buffer.str();
  }

  static std::filesystem::path ssh_path_;
  SSHConnInterface ssh_conn_;
};  // class SshTest

std::filesystem::path SshTest::ssh_path_;

TEST_F(SshTest, SshStdoutCaptured) {
  FakeSsh("echo -n 'Hello World!'");
  auto result = RunCommand("ls");
  ASSERT_OK(result);
  EXPECT_THAT(result->stdout, Eq("Hello World!"));
}

TEST_F(SshTest, SshStderrCaptured) {
  FakeSsh(">&2 echo -n 'Hello World!'");
  auto result = RunCommand("ls");
  ASSERT_OK(result);
  EXPECT_THAT(result->stderr, Eq("Hello World!"));
}

TEST_F(SshTest, SshUserHostExpected) {
  FakeSsh("echo -n $1");
  auto result = RunCommand("ls");
  ASSERT_OK(result);
  EXPECT_THAT(result->exit_code, Eq(0));
  EXPECT_THAT(result->stdout, Eq("root@dut"));
}

TEST_F(SshTest, SshExitCodeCaptured) {
  FakeSsh("exit 42");
  auto result = RunCommand("ls");
  ASSERT_OK(result);
  EXPECT_THAT(result->exit_code, Eq(42));
}

TEST_F(SshTest, SshSignalCaptured) {
  FakeSsh("kill -9 $$");
  auto result = RunCommand("ls");
  ASSERT_OK(result);
  EXPECT_THAT(result->exit_code, Eq(-9));
}

TEST_F(SshTest, SshDeadlineExceeded) {
  FakeSsh("sleep 100");
  auto result = RunCommand("ls");
  EXPECT_THAT(result, StatusIs(absl::StatusCode::kDeadlineExceeded));
}

TEST_F(SshTest, SshStdoutRedirectRejected) {
  auto result =
      RunCommand("ls", ConnInterface::CommandOption{"stdout_file", ""});
  EXPECT_THAT(result, StatusIs(absl::StatusCode::kUnimplemented));
}

TEST_F(SshTest, SshStderrRedirectRejected) {
  auto result =
      RunCommand("ls", ConnInterface::CommandOption{"", "stderr_file"});
  EXPECT_THAT(result, StatusIs(absl::StatusCode::kUnimplemented));
}

TEST_F(SshTest, SshReadFileCallCat) {
  FakeSsh("echo $2; echo -n $3");
  auto result = ReadFile("file_to_read");
  ASSERT_OK(result);
  EXPECT_THAT(*result, Eq(absl::Cord("cat\nfile_to_read")));
}

TEST_F(SshTest, SshReadFileReturnStdoutAsContent) {
  FakeSsh("echo -n \"abc\x01\x05\x0a\x15\"");
  auto result = ReadFile("file_to_read");
  ASSERT_OK(result);
  EXPECT_THAT(*result, Eq(absl::Cord("abc\x01\x05\x0a\x15")));
}

TEST_F(SshTest, SshWriteFileCallCat) {
  // Tmp file to get the stdout of fakessh.
  std::string tmp_file = std::tmpnam(nullptr);
  FakeSsh("echo -n \"$2\n$3\n$4\" >> " + tmp_file);

  auto result = WriteFile("file_to_write", "");

  ASSERT_OK(result);
  std::string actual = ReadLocalFile(tmp_file);
  EXPECT_THAT(actual, Eq("cat\n>\nfile_to_write"));
}

TEST_F(SshTest, SshWriteFileFeedDataToStdin) {
  // Tmp file to get the stdout of fakessh.
  auto tmp_file = std::tmpnam(nullptr);
  FakeSsh(absl::StrCat("cat >> ", tmp_file));

  auto result = WriteFile("file_to_write", "abc\x01\x05\x0a\x15");

  ASSERT_OK(result);
  std::string actual = ReadLocalFile(tmp_file);
  EXPECT_THAT(actual, Eq("abc\x01\x05\x0a\x15"));
}

class SshWithKeyPathTest : public SshTest {
 public:
  SshWithKeyPathTest() : SshTest("ssh_key_path", "") {}
};

TEST_F(SshWithKeyPathTest, SshUserHostExpected) {
  FakeSsh("echo -n $2 $3");
  auto result = RunCommand("ls");
  ASSERT_OK(result);
  EXPECT_THAT(result->exit_code, Eq(0));
  EXPECT_THAT(result->stdout, Eq("-i ssh_key_path"));
}

class SshWithTunnelFilePathTest : public SshTest {
 public:
  SshWithTunnelFilePathTest() : SshTest("", "ssh_tunnel_file_path") {}
};

TEST_F(SshWithTunnelFilePathTest, SshUserHostExpected) {
  FakeSsh("echo -n $2 $3");
  auto result = RunCommand("ls");
  ASSERT_OK(result);
  EXPECT_THAT(result->exit_code, Eq(0));
  EXPECT_THAT(result->stdout, Eq("-S ssh_tunnel_file_path"));
}

}  // namespace remote
}  // namespace ocpdiag::hwinterface
