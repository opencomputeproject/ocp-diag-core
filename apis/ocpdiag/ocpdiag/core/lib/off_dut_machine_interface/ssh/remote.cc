// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/lib/off_dut_machine_interface/ssh/remote.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "absl/cleanup/cleanup.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "absl/types/span.h"
#include "ocpdiag/core/lib/off_dut_machine_interface/remote.h"

namespace ocpdiag::hwinterface {
namespace remote {

constexpr absl::string_view kDefaultSSHUser = "root";
constexpr absl::Duration kPollChildInterval = absl::Milliseconds(100);
constexpr absl::Duration kRWTimeout = absl::Minutes(15);

SSHConnInterface::SSHConnInterface(NodeSpec node_spec,
                                   absl::string_view ssh_key_path,
                                   absl::string_view ssh_tunnel_file_path,
                                   absl::string_view ssh_bin_path)
    : node_spec_(node_spec),
      ssh_key_path_(ssh_key_path),
      ssh_tunnel_file_path_(ssh_tunnel_file_path),
      ssh_bin_path_(ssh_bin_path) {}

absl::StatusOr<absl::Cord> SSHConnInterface::ReadFile(
    absl::string_view file_name) {
  absl::StatusOr<CommandResult> result =
      RunCommand(kRWTimeout, {"cat", file_name}, CommandOption());
  if (!result.ok()) {
    return result.status();
  }
  if (result->exit_code != 0) {
    return absl::InternalError(
        absl::StrFormat("Failed to read the file: %s\nstderr: %s\nstdout: %s.",
                        file_name, result->stderr, result->stdout));
  }
  return absl::Cord(result->stdout);
}

absl::Status SSHConnInterface::WriteFile(absl::string_view file_name,
                                         absl::string_view data) {
  absl::string_view write_file_command[]{"cat", ">", file_name};
  absl::StatusOr<CommandResult> result = RunCommandWithStdin(
      kRWTimeout, data, write_file_command, CommandOption());
  if (!result.ok()) {
    return result.status();
  }
  if (result->exit_code != 0) {
    return absl::InternalError(
        absl::StrFormat("Failed to write the file: %s\nstderr: %s\nstdout: %s.",
                        file_name, result->stderr, result->stdout));
  }
  return absl::OkStatus();
}

absl::StatusOr<ConnInterface::CommandResult> SSHConnInterface::RunCommand(
    absl::Duration timeout, const absl::Span<const absl::string_view> args,
    const ConnInterface::CommandOption& options) {
  return RunCommandWithStdin(timeout, absl::string_view(), args, options);
}

namespace {

// Helper to read the entire contents of a file
absl::StatusOr<std::string> ReadLocalFile(FILE* file) {
  std::fseek(file, 0, SEEK_END);
  std::string out;
  out.resize(std::ftell(file));
  std::rewind(file);
  size_t read = std::fread(out.data(), sizeof(char), out.size(), file);
  if (read < out.size()) {
    return absl::InternalError("Failed to read entirety of file.");
  }
  std::fclose(file);
  return out;
}

}  // namespace

absl::StatusOr<ConnInterface::CommandResult>
SSHConnInterface::RunCommandWithStdin(
    absl::Duration timeout, absl::string_view stdin,
    const absl::Span<const absl::string_view> args,
    const ConnInterface::CommandOption& options) {
  if (!options.stdout_file.empty() || !options.stderr_file.empty()) {
    return absl::UnimplementedError(
        "Stdout/stderr redirection is requested but not yet implemented.");
  }

  // Prepare the stdout and stderr temp files.
  FILE* stdout = std::tmpfile();
  if (stdout == nullptr) {
    return absl::InternalError(absl::StrFormat(
        "Failed to create stdout file for the command: %s.", strerror(errno)));
  }
  FILE* stderr = std::tmpfile();
  if (stderr == nullptr) {
    return absl::InternalError(absl::StrFormat(
        "Failed to create stderr file for the command: %s.", strerror(errno)));
  }

  // Create pipe to route the stdin
  int fd[2];
  if (pipe(fd) != 0) {
    return absl::InternalError(absl::StrFormat(
        "Failed to create pipe for the command: %s.", strerror(errno)));
  }
  auto pipe_cleanup = absl::MakeCleanup([fd] {
    close(fd[0]);
    close(fd[1]);
  });

  pid_t pid = fork();
  if (pid < 0) {
    return absl::InternalError(
        absl::StrFormat("Failed to fork the process: %s.", strerror(errno)));
  }

  // Child process, never returns.
  if (pid == 0) {
    dup2(fileno(stdout), STDOUT_FILENO);
    dup2(fileno(stderr), STDERR_FILENO);

    // Child process doesn't use it.
    close(fd[1]);
    if (dup2(fd[0], STDIN_FILENO) == -1) {
      return absl::InternalError(absl::StrFormat(
          "Failed to use pipe for redirect stdin: %s.", strerror(errno)));
    }

    std::vector<std::string> sshArgs = GenerateSshArg(args);
    //  convert sshArgs to char*const[] required by execvp.
    std::vector<char*> sshArg_ptrs(sshArgs.size() + 1);
    for (std::size_t i = 0; i < sshArgs.size(); i++) {
      sshArg_ptrs[i] = const_cast<char*>(sshArgs[i].c_str());
    }
    sshArg_ptrs.back() = nullptr;
    execvp(ssh_bin_path_.c_str(), sshArg_ptrs.data());

    // Execvp returns only when fail.
    std::cerr << "Failed to run ssh." << std::endl;
    return absl::InternalError(
        absl::StrFormat("Failed to run the command: %s.", strerror(errno)));
  }

  // Close fd[0] after child process inherits it.
  close(fd[0]);
  // Ignore SIGPIPE to avoid program terminate.
  struct sigaction ignore, restore;
  memset(&ignore, 0, sizeof(ignore));
  ignore.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &ignore, &restore);
  auto sigpipe_cleanup = absl::MakeCleanup(
      [&restore]() { sigaction(SIGPIPE, &restore, nullptr); });
  // Write data to pipe for stdin of command.
  for (size_t written = 0; written < stdin.size();) {
    ssize_t wrote =
        write(fd[1], stdin.data() + written, stdin.size() - written);
    if (wrote < 0) {
      return absl::InternalError(absl::StrFormat(
          "Failed to write all data to pipe: %s.", strerror(errno)));
    }
    written += wrote;
  }
  // Close fd[1] to indicate the end of data from pipe.
  close(fd[1]);

  // Below is the parent process code.
  // Wait for the child process to finish, or kill the command if it times out.
  absl::Time deadline = absl::Now() + timeout;
  while (absl::Now() < deadline) {
    int status;
    pid_t w = waitpid(pid, &status, WNOHANG);
    if (w == 0) {
      absl::SleepFor(kPollChildInterval);
      continue;
    }
    if (w != pid) {
      return absl::InternalError("Failed to wait for the child process.");
    }

    // Command finishes successfully. Populate the result.
    ConnInterface::CommandResult result;
    if (WIFEXITED(status)) {
      result.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
      // Exit code is negative when subprocess is terminated by signal.
      result.exit_code = -abs(WTERMSIG(status));
    } else {
      return absl::InternalError(
          "Unexpected state: waitpid returned when subprocess neither exited "
          "nor terminated by signal.");
    }
    // Copy stdout and stderr to the result.
    auto stdout_content = ReadLocalFile(stdout);
    if (!stdout_content.ok()) {
      std::cerr << "Failed to read the stdout." << std::endl;
      return stdout_content.status();
    }
    result.stdout = *std::move(stdout_content);

    auto stderr_content = ReadLocalFile(stderr);
    if (!stderr_content.ok()) {
      std::cerr << "Failed to read the stderr." << std::endl;
      return stderr_content.status();
    }
    result.stderr = *std::move(stderr_content);

    return result;
  }

  // Timeout, kill the command and return.
  kill(pid, SIGKILL);
  return absl::DeadlineExceededError(absl::StrFormat(
      "Command failed to finish in %s.", absl::FormatDuration(timeout)));
}

std::vector<std::string> SSHConnInterface::GenerateSshArg(
    absl::Span<const absl::string_view> args) {
  std::vector<std::string> sshArgs;

  sshArgs.push_back(std::string(ssh_bin_path_));
  sshArgs.push_back(absl::StrCat(kDefaultSSHUser, "@", node_spec_.address));
  if (!ssh_key_path_.empty()) {
    sshArgs.push_back("-i");
    sshArgs.push_back(std::string(ssh_key_path_));
  }
  if (!ssh_tunnel_file_path_.empty()) {
    sshArgs.push_back("-S");
    sshArgs.push_back(std::string(ssh_tunnel_file_path_));
  }
  // Disable SSH host keys checking
  sshArgs.push_back("-o");
  sshArgs.push_back("StrictHostKeyChecking=no");
  sshArgs.push_back("-o");
  sshArgs.push_back("UserKnownHostsFile=/dev/null");
  sshArgs.insert(sshArgs.end(), args.begin(), args.end());
  return sshArgs;
}

}  // namespace remote
}  // namespace ocpdiag::hwinterface
