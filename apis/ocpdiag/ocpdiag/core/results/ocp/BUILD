# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

load("@com_github_grpc_grpc//bazel:python_rules.bzl", "py_proto_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

proto_library(
    name = "results_proto",
    srcs = ["results.proto"],
    deps = [
        "@com_google_protobuf//:struct_proto",
        "@com_google_protobuf//:timestamp_proto",
    ],
)

cc_proto_library(
    name = "results_cc_proto",
    deps = [":results_proto"],
)

py_proto_library(
    name = "results_py_pb2",
    deps = [":results_proto"],
)

cc_library(
    name = "value",
    hdrs = ["value.h"],
)

cc_test(
    name = "value_test",
    srcs = ["value_test.cc"],
    deps = [
        ":value",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "validator",
    hdrs = ["validator.h"],
    deps = [
        ":results_cc_proto",
        ":value",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "validator_test",
    srcs = ["validator_test.cc"],
    deps = [
        ":validator",
        ":value",
        "@com_google_googletest//:gtest_main",
    ],
)
