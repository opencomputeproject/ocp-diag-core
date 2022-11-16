// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ocpdiag/core/params/ocpdiag_parameter_parser.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/util/json_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/base/macros.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "ocpdiag/core/compat/status_converters.h"
#include "ocpdiag/core/params/testdata/dummy_outputs.h"
#include "ocpdiag/core/params/testdata/test_params.pb.h"
#include "ocpdiag/core/params/utils.h"
#include "ocpdiag/core/testing/file_utils.h"
#include "ocpdiag/core/testing/proto_matchers.h"
#include "ocpdiag/core/testing/status_matchers.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"

using ::ocpdiag::testing::EqualsProto;
using testing::AnyOf;
using testing::ElementsAre;
using testing::EndsWith;
using testing::Eq;
using testing::Field;
using testing::HasSubstr;
using testing::Le;
using testing::Ne;
using testing::StartsWith;

namespace ocpdiag {
namespace {

// Helper to read the entire contents of a file.
absl::StatusOr<std::string> ReadFile(FILE* file) {
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

// Helper to convert the params JSON data to a proto.
absl::StatusOr<ocpdiag::core::params::testdata::Params> ParseParams(
    absl::string_view json) {
  ocpdiag::core::params::testdata::Params params;
  if (absl::Status parsed = AsAbslStatus(
          google::protobuf::util::JsonStringToMessage(std::string(json), &params));
      !parsed.ok()) {
    return parsed;
  }
  return params;
}

// Helper to retrieve the test_params.json file data.
std::string GetTestJson() {
  std::string defaults_path = ocpdiag::testutils::GetDataDependencyFilepath(
      "ocpdiag/core/params/testdata/test_params.json");
  std::ifstream defaults{defaults_path};
  std::ostringstream json;
  json << defaults.rdbuf();
  return json.str();
}

// Helper to get test_params.json as a proto.
ocpdiag::core::params::testdata::Params GetTestParams() {
  return *ParseParams(GetTestJson());
}

class OCPDiagParameterParserUnitTest : public ::testing::Test {
 public:
  OCPDiagParameterParserUnitTest()
      : e2e_launcher_(ocpdiag::testutils::GetDataDependencyFilepath(
            "ocpdiag/core/params/ocpdiag_launcher")),
        e2e_binary_(ocpdiag::testutils::GetDataDependencyFilepath(
            "ocpdiag/core/params/testdata/dummy")),
        descriptor_(ocpdiag::testutils::GetDataDependencyFilepath(
            "ocpdiag/core/params/testdata/"
            "test_params_descriptor-transitive-descriptor-set.proto.bin")),
        e2e_(false),
        e2e_ignore_rc_(false),
        e2e_rc_(0) {}

 protected:
  const char* launcher() const { return e2e_launcher_.c_str(); }

  const char* e2e_binary() const { return e2e_binary_.c_str(); }

  const char* descriptor() const { return descriptor_.c_str(); }

  void EnableE2E(bool e2e) { e2e_ = e2e; }

  void E2EIgnoreReturn(bool ignore) { e2e_ignore_rc_ = ignore; }

  int E2EReturnCode() const { return e2e_rc_; }

  // Runs the test by invoking the actual ocpdiag_launcher.
  absl::StatusOr<OCPDiagParameterParser::ExecArgs> RunEndToEnd(
      absl::Span<const char*> argv, absl::string_view json_params) {
    std::vector<const char*> args{argv.begin(), argv.end()};
    args.push_back(nullptr);
    if (absl::string_view{args[0]} != launcher()) {
      return absl::InternalError(
          "End to end parser tests must use launcher() for ocpdiag launcher "
          "argument.");
    }
    if (absl::string_view{args[1]} != e2e_binary()) {
      return absl::InternalError(
          "End to end parser tests must use e2e_binary() for the test "
          "executable argument.");
    }

    // Spawn launcher with STDIN and STDOUT redirected to internal files.
    FILE* stdin = std::tmpfile();
    if (stdin == nullptr) {
      return absl::InternalError(
          "Failed to create temporary json params file.");
    }
    while (!json_params.empty()) {
      int written = std::fwrite(json_params.data(), sizeof(char),
                                json_params.size(), stdin);
      if (written < 0) {
        return absl::InternalError(
            "Failed writing all json params to launcher.");
      }
      json_params.remove_prefix(written);
    }
    std::rewind(stdin);
    std::fflush(stdin);
    FILE* stdout = std::tmpfile();
    if (stdout == nullptr) {
      return absl::InternalError("Failed to create stdout file for launcher.");
    }
    FILE* argout = std::tmpfile();
    if (argout == nullptr) {
      return absl::InternalError("Failed to create arg dump file for test.");
    }
    FILE* stdin_dump = std::tmpfile();
    if (stdin_dump == nullptr) {
      return absl::InternalError("Failed to create stdin dump file for test.");
    }
    pid_t pid = fork();
    if (pid == 0) {
      setenv("OCPDIAG_STDIN", "", /*replace=*/false);
      dup2(fileno(stdin), STDIN_FILENO);
      dup2(fileno(stdout), STDOUT_FILENO);
      dup2(fileno(argout), 3);
      setenv(ocpdiag::kDummyBinaryArgsFileEnvVar, "/proc/self/fd/3", true);
      dup2(fileno(stdin_dump), 4);
      setenv(ocpdiag::kDummyBinaryStdinEnvVar, "/proc/self/fd/4", true);
      // STDERR is shared with the parent so failures are visible.
      execv(args[0], const_cast<char* const*>(args.data()));
      return absl::InternalError(
          absl::StrCat("Failed to exec ocpdiag_launcher (errno: ", errno, ", \"",
                       strerror(errno), "\") @ \"", args[0], "\""));
    }
    std::fclose(stdin);

    // Wait for the launcher to terminate.
    int status;
    if (waitpid(pid, &status, 0) != pid) {
      return absl::InternalError("Failed to wait on forked ocpdiag_launcher.");
    }
    if (!WIFEXITED(status) || (!e2e_ignore_rc_ && WEXITSTATUS(status))) {
      std::string info;
      if (WIFEXITED(status)) {
        info = absl::StrCat(" with error code ", WEXITSTATUS(status));
      }
      if (WIFSIGNALED(status)) {
        info = absl::StrCat(" by signal number ", WTERMSIG(status));
      }
      return absl::UnknownError(
          absl::StrCat("Launcher ended unexpectedly", info));
    }
    e2e_rc_ = WEXITSTATUS(status);

    // Split the output argv into null-terminated character strings.
    absl::StatusOr<std::string> execv_raw = ReadFile(argout);
    if (!execv_raw.ok()) {
      return execv_raw.status();
    }
    execv_e2e_ = *std::move(execv_raw);
    OCPDiagParameterParser::ExecArgs out;
    for (absl::string_view execv_entry = execv_e2e_; !execv_entry.empty();) {
      out.execv.push_back(execv_entry.data());
      size_t end = execv_entry.find_first_of('\0');
      if (end == std::string_view::npos) {
        return absl::InternalError("execv arguments must be null-terminated.");
      }
      execv_entry.remove_prefix(end + 1);
    }

    // Copy the stdin data to the json params.
    absl::StatusOr<std::string> params_raw = ReadFile(stdin_dump);
    if (!params_raw.ok()) {
      return params_raw.status();
    }
    out.json_params = *std::move(params_raw);

    // Copy stdout to the output.
    absl::StatusOr<std::string> output_raw = ReadFile(stdout);
    if (!output_raw.ok()) {
      return output_raw.status();
    }
    out.post_output = *std::move(output_raw);
    return out;
  }

  // Tests parsing args, and then using them to construct the arguments to exec.
  absl::StatusOr<OCPDiagParameterParser::ExecArgs> CreateExecArgs(
      absl::Span<const char*> argv, absl::string_view json_params) {
    if (e2e_) {
      return RunEndToEnd(argv, json_params);
    }
    OCPDiagParameterParser::Arguments args =
        OCPDiagParameterParser::ParseArgs(argv.size(), argv.data());
    google::protobuf::io::ArrayInputStream json_stream{
        json_params.data(), static_cast<int>(json_params.size())};
    return OCPDiagParameterParser::PrepareExec(std::move(args), &json_stream,
                                              /*json_newlines=*/false);
  }

  // Executes the common case of running with TestParams descriptor,
  // test_params.json as default, and some extra arguments.
  absl::StatusOr<OCPDiagParameterParser::ExecArgs> ExecDefaultsWithArgs(
      absl::string_view json_params, absl::Span<const char* const> args) {
    std::string defaults_path = ocpdiag::testutils::GetDataDependencyFilepath(
        "ocpdiag/core/params/testdata/test_params.json");
    std::vector<const char*> argv;
    argv.reserve(4 + args.size());
    argv.push_back(launcher());
    argv.push_back(e2e_binary());
    argv.push_back(descriptor());
    argv.push_back(defaults_path.c_str());
    for (const char* arg : args) {
      argv.push_back(arg);
    }
    return CreateExecArgs(absl::MakeSpan(argv), json_params);
  }

 private:
  const std::string e2e_launcher_;
  const std::string e2e_binary_;
  const std::string descriptor_;
  bool e2e_;
  bool e2e_ignore_rc_;
  int e2e_rc_;
  std::string execv_e2e_;  // Backing store for 'argv'. Must outlive function.
};

class OCPDiagParameterParserTest : public OCPDiagParameterParserUnitTest,
                                  public ::testing::WithParamInterface<bool> {
 public:
  OCPDiagParameterParserTest() { EnableE2E(GetParam()); }
};

INSTANTIATE_TEST_SUITE_P(OCPDiagParameterEndToEndTests,
                         OCPDiagParameterParserTest, ::testing::Bool(),
                         [](const ::testing::TestParamInfo<bool>& info) {
                           return info.param ? "EndToEnd" : "UnitTest";
                         });

TEST_F(OCPDiagParameterParserUnitTest, ArgsParsedProperly) {
  EnableE2E(false);
  const char* argv[] = {"binary_name",
                        "positional_argument",
                        "-foo",
                        "-bar",
                        "--a_really_long_name",
                        "a_really_long_value",
                        "-a_flag=a_value",
                        "--arg",
                        "--following_arg",
                        "--last_arg=final",
                        "stranded_positional_argument",
                        "--",
                        "--flag_for_test",
                        "foo",
                        "bar",
                        "--baz",
                        "/gorp",
                        "--noflag",
                        "--",
                        "moreargs"};
  OCPDiagParameterParser::Arguments args =
      OCPDiagParameterParser::ParseArgs(ABSL_ARRAYSIZE(argv), argv);
  EXPECT_THAT(args.unparsed, ElementsAre("binary_name", "positional_argument",
                                         "stranded_positional_argument"));
  EXPECT_THAT(args.passthrough,
              ElementsAre("--flag_for_test", "foo", "bar", "--baz", "/gorp",
                          "--noflag", "--", "moreargs"));
  EXPECT_THAT(
      args.flags,
      ElementsAre(
          Field(&OCPDiagParameterParser::FlagArg::key, "foo"),
          Field(&OCPDiagParameterParser::FlagArg::key, "bar"),
          Field(&OCPDiagParameterParser::FlagArg::key, "a_really_long_name"),
          Field(&OCPDiagParameterParser::FlagArg::key, "a_flag"),
          Field(&OCPDiagParameterParser::FlagArg::key, "arg"),
          Field(&OCPDiagParameterParser::FlagArg::key, "following_arg"),
          Field(&OCPDiagParameterParser::FlagArg::key, "last_arg")));
  EXPECT_THAT(
      args.flags,
      ElementsAre(
          Field(&OCPDiagParameterParser::FlagArg::value, ""),
          Field(&OCPDiagParameterParser::FlagArg::value, ""),
          Field(&OCPDiagParameterParser::FlagArg::value, "a_really_long_value"),
          Field(&OCPDiagParameterParser::FlagArg::value, "a_value"),
          Field(&OCPDiagParameterParser::FlagArg::value, ""),
          Field(&OCPDiagParameterParser::FlagArg::value, ""),
          Field(&OCPDiagParameterParser::FlagArg::value, "final")));
}

TEST_P(OCPDiagParameterParserTest, SingleMessageDefinitionParses) {
  const char* argv[3] = {launcher(), e2e_binary(), descriptor()};
  auto exec_or = CreateExecArgs(absl::MakeSpan(argv), "");
  EXPECT_OK(exec_or);
}

TEST_P(OCPDiagParameterParserTest, MultiMessageDefinitionParses) {
  const char* argv[3] = {launcher(), e2e_binary(), descriptor()};
  auto exec_or = CreateExecArgs(absl::MakeSpan(argv), "");
  EXPECT_OK(exec_or);
}

TEST_P(OCPDiagParameterParserTest, TestBinaryIsExecArg) {
  const char* argv[3] = {launcher(), e2e_binary(), descriptor()};
  auto exec_or = CreateExecArgs(absl::MakeSpan(argv), "");
  ASSERT_OK(exec_or);
  EXPECT_THAT(std::string{exec_or->execv[0]}, Eq(argv[1]));
}

TEST_P(OCPDiagParameterParserTest, ParamsUnchangedWithoutDefaults) {
  const char* argv[3] = {launcher(), e2e_binary(), descriptor()};
  auto exec_or = CreateExecArgs(absl::MakeSpan(argv), GetTestJson());
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  EXPECT_THAT(*output_or, EqualsProto(GetTestParams()));
}

TEST_P(OCPDiagParameterParserTest, DefaultsProvidedWithoutInput) {
  auto exec_or = ExecDefaultsWithArgs("", absl::Span<const char*>());
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  EXPECT_THAT(*output_or, EqualsProto(GetTestParams()));
}

TEST_P(OCPDiagParameterParserTest, ParamsOverrideDefaults) {
  auto exec_or =
      ExecDefaultsWithArgs("{\"foo\" : \"ABCD\"}", absl::Span<const char*>());
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.set_foo("ABCD");
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, ArgsOverrideDefaults) {
  static constexpr const char* args[] = {"--foo=ABCD"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.set_foo("ABCD");
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, ArgsOverrideParamsOverrideDefaults) {
  static constexpr const char* args[] = {"--foo=WXYZ"};
  auto exec_or =
      ExecDefaultsWithArgs("{\"foo\" : \"ABCD\"}", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.set_foo("WXYZ");
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, SingularNumberOverride) {
  static constexpr const char* args[] = {"--bar=9999999"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.set_bar(9999999);
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, ArrayEntryOverride) {
  static constexpr const char* args[] = {"--strings[1]=injected"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.set_strings(1, "injected");
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, ArrayAssignment) {
  static constexpr const char* args[] = {"--strings=[\"injected\", \"value\"]"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.clear_strings();
  merged.add_strings("injected");
  merged.add_strings("value");
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, MapAssignment) {
  static constexpr const char* args[] = {"--map={\"key\":\"value\"}"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.mutable_map()->insert({"key", "value"});
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, TopLevelMessageArrayAssignment) {
  static constexpr const char* args[] = {
      R"(--subs=[{"sub":"A"}, {"sub":"B"}])"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.clear_subs();
  merged.add_subs()->set_sub("A");
  merged.add_subs()->set_sub("B");
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, NotReallyArrayAssignment) {
  static constexpr const char* args[] = {
      "--strings[1]=[\"injected\", \"value\"]"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  *merged.mutable_strings(1) = "[\"injected\", \"value\"]";
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, MessageAssignment) {
  static constexpr const char* args[] = {"--msg={ \"sub\" : \"test\" }"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  ocpdiag::core::params::testdata::Params::SubParam msg;
  msg.set_sub("test");
  auto merged = GetTestParams();
  *merged.mutable_msg() = msg;
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, NestedEntryOverride) {
  static constexpr const char* args[] = {"--msg.other[3]=54321"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.mutable_msg()->set_other(3, 54321);
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, NestedArrayOverride) {
  static constexpr const char* args[] = {"--subs[1].sub=bogus"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.mutable_subs(1)->set_sub("bogus");
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, EnumNameOverride) {
  static constexpr const char* args[] = {"--enumerated=BAR"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.set_enumerated(
      ocpdiag::core::params::testdata::Params::BAR);
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, EnumNumOverride) {
  static constexpr const char* args[] = {"--enumerated=2"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.set_enumerated(
      ocpdiag::core::params::testdata::Params::BAZ);
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, NameBoolOverride) {
  static constexpr const char* args[] = {"--b=true"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.set_b(true);
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, NumericBoolOverride) {
  static constexpr const char* args[] = {"--b=1"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.set_b(true);
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, SignedInt32Override) {
  static constexpr const char* args[] = {"--i32=-12"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.set_i32(-12);
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, UnsignedInt32Override) {
  static constexpr const char* args[] = {"--u32=12"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.set_u32(12);
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, SignedInt64Override) {
  static constexpr const char* args[] = {"--i64=-12"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.set_i64(-12);
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, UnsignedInt64Override) {
  static constexpr const char* args[] = {"--u64=12"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.set_u64(12);
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, Float32Override) {
  static constexpr const char* args[] = {"--f32=123.456"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.set_f32(123.456);
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, Float64Override) {
  static constexpr const char* args[] = {"--f64=123.456"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.set_f64(123.456);
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

TEST_P(OCPDiagParameterParserTest, HelpTextHasSubstrFields) {
  E2EIgnoreReturn(true);
  static constexpr const char* args[] = {"--help"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  EXPECT_THAT(exec_or->post_output, HasSubstr("--foo "));
  EXPECT_THAT(exec_or->post_output, HasSubstr("--msg "));
  EXPECT_THAT(exec_or->post_output, HasSubstr("--msg.sub "));
  EXPECT_THAT(exec_or->post_output, HasSubstr("--subs "));
  EXPECT_THAT(exec_or->post_output, HasSubstr("--subs[#] "));
  EXPECT_THAT(exec_or->post_output, HasSubstr("--subs[#].sub "));
  EXPECT_THAT(exec_or->post_output, HasSubstr("--subs[#].other "));
  EXPECT_THAT(exec_or->post_output, HasSubstr("--subs[#].other[#] "));
  EXPECT_THAT(exec_or->post_output, HasSubstr("--recursive "));
  EXPECT_THAT(exec_or->post_output, HasSubstr("--recursive.recursive "));
}

TEST_P(OCPDiagParameterParserTest, HelpFullTextHasSubstrFields) {
  E2EIgnoreReturn(true);
  static constexpr const char* args[] = {"--helpfull"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  EXPECT_THAT(exec_or->post_output, HasSubstr("--foo "));
  EXPECT_THAT(exec_or->post_output, HasSubstr("--msg "));
  EXPECT_THAT(exec_or->post_output, HasSubstr("--msg.sub "));
  EXPECT_THAT(exec_or->post_output, HasSubstr("--subs "));
  EXPECT_THAT(exec_or->post_output, HasSubstr("--subs[#] "));
  EXPECT_THAT(exec_or->post_output, HasSubstr("--subs[#].sub "));
  EXPECT_THAT(exec_or->post_output, HasSubstr("--subs[#].other "));
  EXPECT_THAT(exec_or->post_output, HasSubstr("--subs[#].other[#] "));
  EXPECT_THAT(exec_or->post_output, HasSubstr("--recursive "));
  EXPECT_THAT(exec_or->post_output, HasSubstr("--recursive.recursive "));
}

TEST_P(OCPDiagParameterParserTest, HelpTextIndentation) {
  E2EIgnoreReturn(true);
  static constexpr const char* args[] = {"--help"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  EXPECT_THAT(exec_or->post_output, HasSubstr("\n  Environment Variables:\n"));
  EXPECT_THAT(exec_or->post_output, HasSubstr("\n  Default Parameters:\n"));
  absl::string_view output = exec_or->post_output;
  size_t split = output.find("Environment Variables:");
  absl::string_view flags = output.substr(0, split);
  flags = flags.substr(0, flags.find_last_not_of("\n\r\t "));
  for (auto line : absl::StrSplit(flags, absl::ByChar('\n'))) {
    // Each line must be a header, an empty line, a flag declaration, or a
    // multi-line indent.
    EXPECT_THAT(std::string(line),
                AnyOf(AllOf(StartsWith("  Flags from"), EndsWith(":")), Eq(""),
                      StartsWith("    --"), StartsWith("      ")));
    // Lines should wrap at 80 columns.
    EXPECT_THAT(line.size(), Le(80));
  }
  // Test environment indentation.
  absl::string_view environment = output.substr(split);
  environment = environment.substr(environment.find_first_of('\n'));
  split = environment.find("  Default Parameters:");
  environment = environment.substr(0, split);
  for (auto line : absl::StrSplit(environment, absl::ByChar('\n'))) {
    EXPECT_THAT(std::string(line), AnyOf(StartsWith("    "), Eq("")));
  }
  // Test default parameter indentation.
  split = output.find("Default Parameters:");
  absl::string_view defaults = output.substr(split);
  defaults = defaults.substr(defaults.find_first_of('\n'));
  for (auto line : absl::StrSplit(defaults, absl::ByChar('\n'))) {
    EXPECT_THAT(std::string(line), AnyOf(StartsWith("    "), Eq("")));
  }
  // Trailing newline.
  EXPECT_THAT(exec_or->post_output.back(), Eq('\n'));
}

TEST_P(OCPDiagParameterParserTest, DefaultsValidJson) {
  E2EIgnoreReturn(true);
  static constexpr const char* args[] = {"--help"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  EXPECT_THAT(exec_or->post_output, HasSubstr("Default Parameters:\n"));
  absl::string_view defaults = exec_or->post_output;
  defaults = defaults.substr(defaults.find("Default Parameters:"));
  defaults = defaults.substr(defaults.find_first_of('\n'));
  EXPECT_OK(ParseParams(defaults));
}

TEST_F(OCPDiagParameterParserUnitTest, DryRunStdOutContainsOverrides) {
  EnableE2E(true);
  E2EIgnoreReturn(true);
  static constexpr const char* args[] = {"--dry_run", "--i64=-124"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  EXPECT_THAT(exec_or->post_output, Ne(""));
  auto dry_run_or = ParseParams(exec_or->post_output);
  ASSERT_OK(dry_run_or);
  auto merged = GetTestParams();
  merged.set_i64(-124);
  EXPECT_THAT(*dry_run_or, EqualsProto(merged));
}

TEST_F(OCPDiagParameterParserUnitTest, VersionArgOutputs) {
  EnableE2E(true);
  E2EIgnoreReturn(true);
  static constexpr const char* args[] = {"--version"};
  auto exec_or = ExecDefaultsWithArgs("", absl::MakeSpan(args));
  ASSERT_OK(exec_or);
  EXPECT_TRUE(absl::StartsWith(exec_or->post_output, "Version:"));
}

TEST_P(OCPDiagParameterParserTest, UserParamsZeroValueMergesOverNonZeroDefault) {
  auto exec_or =
      ExecDefaultsWithArgs("{\"b\" : false}", absl::Span<const char*>());
  ASSERT_OK(exec_or);
  auto output_or = ParseParams(exec_or->json_params);
  ASSERT_OK(output_or);
  auto merged = GetTestParams();
  merged.set_b(false);
  EXPECT_THAT(*output_or, EqualsProto(merged));
}

}  // namespace
}  // namespace ocpdiag
