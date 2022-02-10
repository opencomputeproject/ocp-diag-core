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

# Metlan simple example.
load("//ocpdiag/core:ocpdiag.bzl", "ocpdiag_test_pkg")

licenses(["notice"])

proto_library(
    name = "params_proto",
    srcs = ["params.proto"],
    deps = [
        "//ocpdiag/core/params:ocpdiag_params_proto",
    ],
)

cc_proto_library(
    name = "params_cc_proto",
    deps = [":params_proto"],
)

cc_library(
    name = "simple_lib",
    srcs = ["simple_lib.cc"],
    hdrs = ["simple_lib.h"],
    deps = [
        "//ocpdiag/core/params:utils",
        "//ocpdiag/core/results",
        "@com_google_absl//absl/types:span",
    ],
)

cc_test(
    name = "simple_lib_test",
    srcs = ["simple_lib_test.cc"],
    deps = [
        ":params_cc_proto",
        ":simple_lib",
        "//ocpdiag/core/compat:status_converters",
        "//ocpdiag/core/results",
        "//ocpdiag/core/results:results_cc_proto",
        "//ocpdiag/core/testing:fake_test_run",
        "//ocpdiag/core/testing:proto_matchers",
        "//ocpdiag/core/testing:status_matchers",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_binary(
    name = "simple_failure_bin",
    srcs = ["simple.cc"],
    deps = [
        ":params_cc_proto",
        ":simple_lib",
        "//ocpdiag/core/params:utils",
        "//ocpdiag/core/results",
        "//ocpdiag/core/results:results_cc_proto",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
    ],
)

ocpdiag_test_pkg(
    name = "simple_failure",
    binary = ":simple_failure_bin",
    json_defaults = "params.json",
    params_proto = ":params_proto",
)
