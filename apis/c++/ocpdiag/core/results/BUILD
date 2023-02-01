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
    name = "variant",
    hdrs = ["variant.h"],
    deps = ["@com_google_absl//absl/strings"],
)

cc_test(
    name = "variant_test",
    srcs = ["variant_test.cc"],
    deps = [
        ":variant",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "structs",
    hdrs = ["structs.h"],
    deps = [
        ":variant",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
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
    name = "struct_validators",
    srcs = ["struct_validators.cc"],
    hdrs = ["struct_validators.h"],
    deps = [
        ":structs",
        ":variant",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "struct_validators_test",
    srcs = ["struct_validators_test.cc"],
    deps = [
        ":struct_validators",
        ":structs",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "int_incrementer",
    hdrs = ["int_incrementer.h"],
    deps = ["@com_google_absl//absl/synchronization"],
)

cc_library(
    name = "dut_info",
    srcs = ["dut_info.cc"],
    hdrs = ["dut_info.h"],
    deps = [
        ":int_incrementer",
        ":struct_validators",
        ":structs",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
    ],
)

cc_test(
    name = "dut_info_test",
    srcs = ["dut_info_test.cc"],
    deps = [
        ":dut_info",
        ":structs",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "proto_converters",
    srcs = ["proto_converters.cc"],
    hdrs = ["proto_converters.h"],
    deps = [
        ":dut_info",
        ":results_cc_proto",
        ":structs",
        ":variant",
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
        ":dut_info",
        ":proto_converters",
        ":results_cc_proto",
        ":structs",
        "//ocpdiag/core/testing:parse_text_proto",
        "//ocpdiag/core/testing:proto_matchers",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "test_result_calculator",
    srcs = ["test_result_calculator.cc"],
    hdrs = ["test_result_calculator.h"],
    deps = [
        ":structs",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/synchronization",
    ],
)

cc_test(
    name = "test_result_calculator_test",
    srcs = [
        "test_result_calculator_test.cc",
    ],
    deps = [
        ":results_cc_proto",
        ":test_result_calculator",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "artifact_writer",
    srcs = ["artifact_writer.cc"],
    hdrs = ["artifact_writer.h"],
    deps = [
        ":int_incrementer",
        ":results_cc_proto",
        "//ocpdiag/core/compat:status_converters",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
        "@com_google_protobuf//:protobuf",
        "@com_google_riegeli//riegeli/base:object",
        "@com_google_riegeli//riegeli/bytes:fd_writer",
        "@com_google_riegeli//riegeli/records:record_writer",
        "@com_google_riegeli//riegeli/records:records_metadata_cc_proto",
    ],
)

cc_test(
    name = "artifact_writer_test",
    srcs = [
        "artifact_writer_test.cc",
    ],
    deps = [
        ":artifact_writer",
        ":results_cc_proto",
        "//ocpdiag/core/testing:file_utils",
        "//ocpdiag/core/testing:proto_matchers",
        "//ocpdiag/core/testing:status_matchers",
        "@com_google_absl//absl/cleanup",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@com_google_riegeli//riegeli/bytes:fd_reader",
        "@com_google_riegeli//riegeli/records:record_reader",
    ],
)

cc_library(
    name = "output_iterator",
    hdrs = ["output_iterator.h"],
    deps = [
        ":proto_converters",
        ":results_cc_proto",
        ":structs",
        "@com_google_absl//absl/log:check",
        "@com_google_riegeli//riegeli/bytes:fd_reader",
        "@com_google_riegeli//riegeli/records:record_reader",
    ],
)

cc_test(
    name = "output_iterator_test",
    srcs = ["output_iterator_test.cc"],
    deps = [
        ":output_iterator",
        ":results_cc_proto",
        "//ocpdiag/core/testing:file_utils",
        "//ocpdiag/core/testing:parse_text_proto",
        "@com_google_googletest//:gtest_main",
        "@com_google_riegeli//riegeli/bytes:fd_writer",
        "@com_google_riegeli//riegeli/records:record_writer",
    ],
)

cc_library(
    name = "output_receiver",
    testonly = 1,
    srcs = ["output_receiver.cc"],
    hdrs = ["output_receiver.h"],
    deps = [
        ":artifact_writer",
        ":output_iterator",
        ":results_cc_proto",
        ":structs",
        "//ocpdiag/core/testing:file_utils",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
    ],
)

cc_test(
    name = "output_receiver_test",
    srcs = ["output_receiver_test.cc"],
    deps = [
        ":artifact_writer",
        ":output_receiver",
        ":proto_converters",
        ":results_cc_proto",
        ":structs",
        "//ocpdiag/core/testing:parse_text_proto",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "log_sink",
    hdrs = ["log_sink.h"],
    deps = [
        ":artifact_writer",
        ":results_cc_proto",
        "@com_google_absl//absl/base:log_severity",
        "@com_google_absl//absl/log:log_entry",
        "@com_google_absl//absl/log:log_sink",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "log_sink_test",
    srcs = ["log_sink_test.cc"],
    deps = [
        ":log_sink",
        ":output_receiver",
        ":structs",
        "@com_google_absl//absl/base:log_severity",
        "@com_google_absl//absl/log",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "test_run",
    srcs = ["test_run.cc"],
    hdrs = ["test_run.h"],
    deps = [
        ":artifact_writer",
        ":dut_info",
        ":int_incrementer",
        ":log_sink",
        ":proto_converters",
        ":results_cc_proto",
        ":struct_validators",
        ":structs",
        ":test_result_calculator",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/log:log_sink_registry",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
    ],
)

cc_test(
    name = "test_run_test",
    srcs = ["test_run_test.cc"],
    deps = [
        ":artifact_writer",
        ":dut_info",
        ":output_receiver",
        ":structs",
        ":test_run",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "test_step",
    srcs = ["test_step.cc"],
    hdrs = ["test_step.h"],
    deps = [
        ":artifact_writer",
        ":proto_converters",
        ":results_cc_proto",
        ":struct_validators",
        ":structs",
        ":test_run",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
    ],
)

cc_test(
    name = "test_step_test",
    srcs = ["test_step_test.cc"],
    deps = [
        ":dut_info",
        ":output_receiver",
        ":structs",
        ":test_run",
        ":test_step",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "measurement_series",
    srcs = ["measurement_series.cc"],
    hdrs = ["measurement_series.h"],
    deps = [
        ":artifact_writer",
        ":int_incrementer",
        ":proto_converters",
        ":results_cc_proto",
        ":struct_validators",
        ":structs",
        ":test_step",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/synchronization",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "measurement_series_test",
    srcs = ["measurement_series_test.cc"],
    deps = [
        ":dut_info",
        ":measurement_series",
        ":output_receiver",
        ":structs",
        ":test_run",
        ":test_step",
        "@com_google_absl//absl/log:check",
        "@com_google_googletest//:gtest_main",
    ],
)
