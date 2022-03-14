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
    name = "file_utils",
    testonly = True,
    srcs = ["file_utils.cc"],
    hdrs = ["file_utils.h"],
    deps = [
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

py_library(
    name = "file_utils_py",
    testonly = True,
    srcs = ["file_utils.py"],
)

cc_library(
    name = "proto_matchers",
    testonly = True,
    srcs = ["proto_matchers.cc"],
    hdrs = ["proto_matchers.h"],
    deps = [
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_googletest//:gtest",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "parse_text_proto",
    testonly = True,
    hdrs = ["parse_text_proto.h"],
    deps = [
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "mock_results",
    testonly = True,
    hdrs = ["mock_results.h"],
    deps = [
        "//ocpdiag/core/hwinterface/lib/off_dut_machine_interface:mock_remote_cc",
        "//ocpdiag/core/results",
        "//ocpdiag/core/results:results_cc_proto",
        "//ocpdiag/core/results/internal:logging",
        "//ocpdiag/core/results/internal:mock_file_handler",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "status_matchers",
    testonly = True,
    srcs = ["status_matchers.cc"],
    hdrs = ["status_matchers.h"],
    deps = [
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:any",
        "@com_google_absl//absl/utility",
        "@com_google_googletest//:gtest",
    ],
)
