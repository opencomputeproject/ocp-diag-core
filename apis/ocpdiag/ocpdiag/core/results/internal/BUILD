# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "utils",
    hdrs = ["utils.h"],
    deps = ["@com_google_absl//absl/synchronization"],
)

cc_library(
    name = "test_utils",
    testonly = True,
    hdrs = ["test_utils.h"],
    deps = ["@com_google_googletest//:gtest"],
)

cc_library(
    name = "logging",
    srcs = ["logging.cc"],
    hdrs = ["logging.h"],
    deps = [
        "//ocpdiag/core/compat:status_converters",
        "//ocpdiag/core/results:results_cc_proto",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/cleanup",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
        "@com_google_protobuf//:protobuf",
        "@com_google_riegeli//riegeli/base:object",
        "@com_google_riegeli//riegeli/bytes:fd_reader",
        "@com_google_riegeli//riegeli/bytes:fd_writer",
        "@com_google_riegeli//riegeli/records:record_reader",
        "@com_google_riegeli//riegeli/records:record_writer",
        "@com_google_riegeli//riegeli/records:records_metadata_cc_proto",
    ],
)

cc_library(
    name = "file_handler",
    srcs = ["file_handler.cc"],
    hdrs = ["file_handler.h"],
    deps = [
        "//ocpdiag/core/lib/off_dut_machine_interface:remote_cc",
        "//ocpdiag/core/lib/off_dut_machine_interface:remote_factory_cc",
        "//ocpdiag/core/results:results_cc_proto",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_library(
    name = "mock_file_handler",
    testonly = True,
    hdrs = ["mock_file_handler.h"],
    deps = [
        ":file_handler",
        "//ocpdiag/core/lib/off_dut_machine_interface:mock_remote_cc",
        "//ocpdiag/core/lib/off_dut_machine_interface:remote_cc",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
    ],
)

cc_test(
    name = "logging_test",
    srcs = ["logging_test.cc"],
    deps = [
        ":logging",
        ":test_utils",
        "//ocpdiag/core/compat:status_converters",
        "//ocpdiag/core/results:results_cc_proto",
        "//ocpdiag/core/testing:proto_matchers",
        "//ocpdiag/core/testing:status_matchers",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
        "@com_google_riegeli//riegeli/bytes:fd_reader",
        "@com_google_riegeli//riegeli/records:record_reader",
    ],
)

cc_test(
    name = "file_handler_test",
    srcs = ["file_handler_test.cc"],
    deps = [
        ":file_handler",
        ":mock_file_handler",
        "//ocpdiag/core/lib/off_dut_machine_interface:mock_remote_cc",
        "//ocpdiag/core/lib/off_dut_machine_interface:remote_cc",
        "//ocpdiag/core/testing:parse_text_proto",
        "//ocpdiag/core/testing:proto_matchers",
        "//ocpdiag/core/testing:status_matchers",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_googletest//:gtest_main",
    ],
)
