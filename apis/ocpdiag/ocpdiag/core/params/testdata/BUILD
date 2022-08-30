# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

# Contains definitions for testing OCPDiag parameter parsing.

# This rule will need to be substituted depending on the OSS build system.
# We can make a stripped-down copy for bazel, or a direct protoc flag
# (--descriptor_set_out) in the makefile proto_library equivalent.
load("//ocpdiag:packaging.bzl", "descriptor_set")
load("@com_github_grpc_grpc//bazel:python_rules.bzl", "py_proto_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

exports_files(
    srcs = [
        "bad_test_params.json",
        "test_params.json",
    ],
)

proto_library(
    name = "ambiguous_params",
    testonly = True,
    srcs = ["ambiguous_params.proto"],
    deps = ["//ocpdiag/core/params:ocpdiag_params_proto"],
)

cc_proto_library(
    name = "ambiguous_params_cc_proto",
    testonly = True,
    deps = [":ambiguous_params"],
)

descriptor_set(
    name = "ambiguous_params_descriptor",
    testonly = True,
    deps = ["ambiguous_params"],
)

proto_library(
    name = "test_params",
    testonly = True,
    srcs = ["test_params.proto"],
)

cc_proto_library(
    name = "test_params_cc_proto",
    testonly = True,
    deps = [":test_params"],
)

py_proto_library(
    name = "test_params_py_pb2",
    testonly = True,
    deps = [":test_params"],
)

descriptor_set(
    name = "test_params_descriptor",
    testonly = True,
    deps = ["test_params"],
)

cc_binary(
    name = "help_only",
    srcs = ["help_only.cc"],
    deps = [
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "dummy_outputs",
    testonly = True,
    hdrs = ["dummy_outputs.h"],
)

cc_binary(
    name = "dummy",
    testonly = True,
    srcs = ["dummy.cc"],
    deps = [":dummy_outputs"],
)
