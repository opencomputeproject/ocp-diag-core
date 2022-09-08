# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

load("//ocpdiag:python.bzl", "pytype_strict_library", pytype_strict_test = "pytype_strict_contrib_test")
load("//ocpdiag:params_version.bzl", "get_params_version_command")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

proto_library(
    name = "ocpdiag_params_proto",
    srcs = ["ocpdiag_params.proto"],
    deps = ["@com_google_protobuf//:descriptor_proto"],
)

cc_proto_library(
    name = "ocpdiag_params_cc_proto",
    visibility = ["//visibility:private"],
    deps = [":ocpdiag_params_proto"],
)

cc_library(
    name = "ocpdiag_parameter_parser",
    srcs = ["ocpdiag_parameter_parser.cc"],
    hdrs = ["ocpdiag_parameter_parser.h"],
    deps = [
        ":ocpdiag_params_cc_proto",
        ":utils",
        "//ocpdiag/core/compat:status_converters",
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
    name = "version_file",
    outs = ["utils_version.h"],
    cmd = get_params_version_command(),
    stamp = 1,
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
        "//ocpdiag/core/compat:status_converters",
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
        "//ocpdiag/core/params/testdata:bad_test_params.json",
        "//ocpdiag/core/params/testdata:test_params.json",
    ],
    deps = [
        ":utils",
        "//ocpdiag/core/params/testdata:test_params_cc_proto",
        "//ocpdiag/core/testing:file_utils",
        "//ocpdiag/core/testing:status_matchers",
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
        "//ocpdiag/core/params/testdata:test_params_py_pb2",
    ],
)

cc_test(
    name = "ocpdiag_parameter_parser_test",
    srcs = ["ocpdiag_parameter_parser_test.cc"],
    data = [
        ":ocpdiag_launcher",
        "//ocpdiag/core/params/testdata:ambiguous_params_descriptor",
        "//ocpdiag/core/params/testdata:dummy",
        "//ocpdiag/core/params/testdata:test_params.json",
        "//ocpdiag/core/params/testdata:test_params_descriptor",
    ],
    deps = [
        ":ocpdiag_parameter_parser",
        ":utils",
        "//ocpdiag/core/compat:status_converters",
        "//ocpdiag/core/params/testdata:dummy_outputs",
        "//ocpdiag/core/params/testdata:test_params_cc_proto",
        "//ocpdiag/core/testing:file_utils",
        "//ocpdiag/core/testing:proto_matchers",
        "//ocpdiag/core/testing:status_matchers",
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
        "//ocpdiag/core/compat:status_converters",
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
        "//ocpdiag/core/params/testdata:ambiguous_params_cc_proto",
        "//ocpdiag/core/params/testdata:test_params_cc_proto",
        "//ocpdiag/core/testing:proto_matchers",
        "//ocpdiag/core/testing:status_matchers",
        "@com_google_absl//absl/status",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_binary(
    name = "ocpdiag_launcher",
    srcs = ["ocpdiag_parameters_main.cc"],
    visibility = ["//visibility:public"],
    deps = [
        ":ocpdiag_parameter_parser",
        ":utils",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
        "@com_google_protobuf//:protobuf_lite",
    ],
)
