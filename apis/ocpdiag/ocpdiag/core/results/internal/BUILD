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
        "//ocpdiag/core/params:utils",
        "//ocpdiag/core/results:results_cc_proto",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "file_handler",
    srcs = ["file_handler.cc"],
    hdrs = ["file_handler.h"],
    deps = [
        "//ocpdiag/core/hwinterface/lib/off_dut_machine_interface:remote_cc",
        "//ocpdiag/core/hwinterface/lib/off_dut_machine_interface:remote_factory_cc",
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
        "//ocpdiag/core/hwinterface/lib/off_dut_machine_interface:mock_remote_cc",
        "//ocpdiag/core/hwinterface/lib/off_dut_machine_interface:remote_cc",
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
    ],
)

cc_test(
    name = "file_handler_test",
    srcs = ["file_handler_test.cc"],
    deps = [
        ":file_handler",
        ":mock_file_handler",
        "//ocpdiag/core/hwinterface/lib/off_dut_machine_interface:mock_remote_cc",
        "//ocpdiag/core/hwinterface/lib/off_dut_machine_interface:remote_cc",
        "//ocpdiag/core/testing:parse_text_proto",
        "//ocpdiag/core/testing:proto_matchers",
        "//ocpdiag/core/testing:status_matchers",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)
