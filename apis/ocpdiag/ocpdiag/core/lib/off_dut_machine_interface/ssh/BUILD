# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

# Common ocpdiag components rules.
load("//ocpdiag:python.bzl", "pytype_strict_library", pytype_strict_test = "pytype_strict_contrib_test")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

cc_library(
    name = "remote_ssh_cc",
    srcs = [
        "remote.cc",
    ],
    hdrs = [
        "remote.h",
    ],
    deps = [
        "//ocpdiag/core/lib/off_dut_machine_interface:remote_cc",
        "@com_google_absl//absl/cleanup",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
        "@com_google_absl//absl/types:span",
    ],
)

cc_test(
    name = "remote_ssh_cc_test",
    srcs = ["remote_test.cc"],
    deps = [
        ":remote_ssh_cc",
        "//ocpdiag/core/lib/off_dut_machine_interface:remote_cc",
        "//ocpdiag/core/testing:status_matchers",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_absl//absl/types:span",
        "@com_google_googletest//:gtest_main",
    ],
)

pytype_strict_library(
    name = "remote_ssh_py",
    srcs = ["remote.py"],
    srcs_version = "PY3",
    deps = [
        "//ocpdiag/core/lib/off_dut_machine_interface:remote_py",
        "@com_google_absl_py//absl/logging",
    ],
)

pytype_strict_test(
    name = "remote_ssh_py_test",
    size = "small",
    srcs = ["remote_test.py"],
    main = "remote_test.py",
    python_version = "PY3",
    deps = [
        ":remote_ssh_py",
        "//ocpdiag/core/lib/off_dut_machine_interface:remote_py",
        "@com_google_absl_py//absl/testing:parameterized",
    ],
)
