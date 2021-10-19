# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

load("//meltan:python.bzl", "pytype_strict_library", pytype_strict_test = "pytype_strict_contrib_test")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

proto_library(
    name = "meltan_params_proto",
    srcs = ["meltan_params.proto"],
    deps = ["@com_google_protobuf//:descriptor_proto"],
)

cc_proto_library(
    name = "meltan_params_cc_proto",
    visibility = ["//visibility:private"],
    deps = [":meltan_params_proto"],
)

cc_library(
    name = "meltan_parameter_parser",
    srcs = ["meltan_parameter_parser.cc"],
    hdrs = ["meltan_parameter_parser.h"],
    deps = [
        ":meltan_params_cc_proto",
        ":utils",
        "//meltan/core/compat:status_converters",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:cord",
        "@com_google_absl//absl/types:span",
        "@com_google_protobuf//:protobuf",
    ],
)

genrule(
    name = "version_header",
    srcs = ["meltan_version.txt"],
    outs = ["utils_version.h"],
    cmd = """cat >$@ <(echo -n 'constexpr char kVersionString[] = R"ENDOFVERSION(') $< <(echo ')ENDOFVERSION";')""",
    visibility = ["//visibility:private"],
)

cc_library(
    name = "utils",
    srcs = [
        "utils.cc",
        "utils_version.h",
    ],
    hdrs = ["utils.h"],
    deps = [
        "//meltan/core/compat:status_converters",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "utils_cc_test",
    srcs = ["utils_test.cc"],
    data = [
        "//meltan/lib/params/testdata:bad_test_params.json",
        "//meltan/lib/params/testdata:test_params.json",
    ],
    deps = [
        ":utils",
        "//meltan/lib/params/testdata:test_params_cc_proto",
        "//meltan/lib/testing:file_utils",
        "//meltan/lib/testing:status_matchers",
        "@com_google_absl//absl/status",
        "@com_google_googletest//:gtest_main",
    ],
)

pytype_strict_library(
    name = "utils_py",
    srcs = ["utils.py"],
    srcs_version = "PY3",
    deps = ["@com_google_protobuf//:protobuf_python"],
)

pytype_strict_test(
    name = "utils_test",
    srcs = ["utils_test.py"],
    python_version = "PY3",
    deps = [
        ":utils_py",
        "//meltan/lib/params/testdata:test_params_py_pb2",
    ],
)

cc_test(
    name = "meltan_parameter_parser_test",
    srcs = ["meltan_parameter_parser_test.cc"],
    data = [
        ":meltan_launcher",
        "//meltan/lib/params/testdata:ambiguous_params_descriptor",
        "//meltan/lib/params/testdata:dummy",
        "//meltan/lib/params/testdata:test_params.json",
        "//meltan/lib/params/testdata:test_params_descriptor",
    ],
    deps = [
        ":meltan_parameter_parser",
        ":utils",
        "//meltan/core/compat:status_converters",
        "//meltan/lib/params/testdata:dummy_outputs",
        "//meltan/lib/params/testdata:test_params_cc_proto",
        "//meltan/lib/testing:file_utils",
        "//meltan/lib/testing:proto_matchers",
        "//meltan/lib/testing:status_matchers",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_library(
    name = "fake_params",
    testonly = True,
    srcs = ["fake_params.cc"],
    hdrs = ["fake_params.h"],
    deps = [
        "//meltan/core/compat:status_converters",
        "@com_google_absl//absl/cleanup",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "fake_params_test",
    srcs = ["fake_params_test.cc"],
    deps = [
        ":fake_params",
        ":utils",
        "//meltan/lib/params/testdata:ambiguous_params_cc_proto",
        "//meltan/lib/params/testdata:test_params_cc_proto",
        "//meltan/lib/testing:proto_matchers",
        "//meltan/lib/testing:status_matchers",
        "@com_google_absl//absl/status",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_binary(
    name = "meltan_launcher",
    srcs = ["meltan_parameters_main.cc"],
    visibility = ["//visibility:public"],
    deps = [
        ":meltan_parameter_parser",
        ":utils",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
        "@com_google_protobuf//:protobuf_lite",
    ],
)
