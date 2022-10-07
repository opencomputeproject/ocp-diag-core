# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

# OCPDiag simple example.
load("//ocpdiag:packaging.bzl", "ocpdiag_test_pkg")

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
        "//ocpdiag/core/results:output_receiver",
        "//ocpdiag/core/results:results_cc_proto",
        "//ocpdiag/core/testing:mock_results",
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
