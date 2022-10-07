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
        "//ocpdiag/core/results/internal:file_handler",
        "//ocpdiag/core/results/internal:logging",
        "//ocpdiag/core/results/internal:utils",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/cleanup",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/log:log_entry",
        "@com_google_absl//absl/log:log_sink",
        "@com_google_absl//absl/log:log_sink_registry",
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
        ":recordio_iterator",
        ":results",
        ":results_cc_proto",
        "//ocpdiag/core/compat:status_converters",
        "//ocpdiag/core/results/internal:logging",
        "//ocpdiag/core/results/internal:mock_file_handler",
        "//ocpdiag/core/testing:parse_text_proto",
        "//ocpdiag/core/testing:proto_matchers",
        "//ocpdiag/core/testing:status_matchers",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/flags:reflection",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/synchronization",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "output_model",
    srcs = ["output_model.cc"],
    hdrs = ["output_model.h"],
    deps = [
        ":results_cc_proto",
        "//ocpdiag/core/compat:status_macros",
        "@com_google_absl//absl/cleanup",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/functional:bind_front",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_test(
    name = "output_model_test",
    srcs = [
        "output_model_test.cc",
    ],
    deps = [
        ":output_model",
        ":results_cc_proto",
        "//ocpdiag/core/testing:parse_text_proto",
        "//ocpdiag/core/testing:status_matchers",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "output_receiver",
    testonly = 1,
    srcs = ["output_receiver.cc"],
    hdrs = ["output_receiver.h"],
    deps = [
        ":output_model",
        ":recordio_iterator",
        ":results",
        ":results_cc_proto",
        "//ocpdiag/core/results/internal:logging",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_test(
    name = "output_receiver_test",
    srcs = [
        "output_receiver_test.cc",
    ],
    deps = [
        ":output_receiver",
        ":results",
        ":results_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "recordio_iterator",
    testonly = 1,
    hdrs = ["recordio_iterator.h"],
    deps = [
        "@com_google_absl//absl/log:check",
        "@com_google_riegeli//riegeli/bytes:fd_reader",
        "@com_google_riegeli//riegeli/records:record_reader",
    ],
)

cc_test(
    name = "recordio_iterator_test",
    srcs = [
        "recordio_iterator_test.cc",
    ],
    deps = [
        ":recordio_iterator",
        ":results_cc_proto",
        "@com_google_googletest//:gtest_main",
        "@com_google_riegeli//riegeli/bytes:fd_writer",
        "@com_google_riegeli//riegeli/records:record_writer",
    ],
)
