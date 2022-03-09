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
        "//ocpdiag/core/hwinterface/lib/off_dut_machine_interface:remote_cc",
        "@com_google_absl//absl/base",
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
        "//ocpdiag/core/hwinterface/lib/off_dut_machine_interface:remote_cc",
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
        "//ocpdiag/core/hwinterface/lib/off_dut_machine_interface:remote_py",
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
        "//ocpdiag/core/hwinterface/lib/off_dut_machine_interface:remote_py",
        "@com_google_absl_py//absl/testing:parameterized",
    ],
)
