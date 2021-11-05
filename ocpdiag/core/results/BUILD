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

load("@com_github_grpc_grpc//bazel:python_rules.bzl", "py_proto_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

proto_library(
    name = "results_proto",
    srcs = ["results.proto"],
    deps = [
        "@com_google_protobuf//:any_proto",
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
    name = "results",
    srcs = ["results.cc"],
    hdrs = ["results.h"],
    deps = [
        ":results_cc_proto",
        "//ocpdiag/core/compat:status_macros",
        "//ocpdiag/core/params:utils",
        "//ocpdiag/core/results/internal:logging",
        "//ocpdiag/core/results/internal:utils",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/types:span",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "results_test",
    srcs = ["results_test.cc"],
    deps = [
        ":results",
        ":results_cc_proto",
        "//ocpdiag/core/compat:status_converters",
        "//ocpdiag/core/results/internal:logging",
        "//ocpdiag/core/results/internal:test_utils",
        "//ocpdiag/core/testing:mock_results",
        "//ocpdiag/core/testing:parse_text_proto",
        "//ocpdiag/core/testing:proto_matchers",
        "//ocpdiag/core/testing:status_matchers",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/synchronization",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)
