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
    name = "structs",
    hdrs = ["structs.h"],
    deps = [
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "structs_test",
    srcs = ["structs_test.cc"],
    deps = [
        ":structs",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "proto_converters",
    srcs = ["proto_converters.cc"],
    hdrs = ["proto_converters.h"],
    deps = [
        ":results_cc_proto",
        ":structs",
        "//ocpdiag/core/compat:status_converters",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "proto_converters_test",
    srcs = ["proto_converters_test.cc"],
    deps = [
        ":proto_converters",
        ":structs",
        "//ocpdiag/core/testing:proto_matchers",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)
