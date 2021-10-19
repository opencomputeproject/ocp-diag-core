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

# Description:
#   BUILD file for the clif example illustrating wrapping inheritance

load("//meltan:python.bzl", "pytype_pybind_extension", "pytype_strict_library")
load("//meltan:python.bzl", "py_strict_test")

licenses(["notice"])

pytype_strict_library(
    name = "results",
    srcs = ["results.py"],
    data = [":_results.so"],
    visibility = ["//visibility:public"],
    deps = [
        "//meltan/lib/results:results_py_pb2",  # buildcleaner: keep
        "@com_google_absl_py//absl:app",
        "@com_google_absl_py//absl/flags",
        "@com_google_protobuf//:protobuf_python",  # buildcleaner: keep
    ],
)

pytype_pybind_extension(
    name = "_results",
    srcs = ["results.cc"],
    pytype_srcs = ["_results.pyi"],
    deps = [
        "//meltan/lib/results",
        "//meltan/lib/results:results_cc_proto",
        "//meltan/lib/results/internal:logging",
        "@com_google_absl//absl/flags:flag",
        "@pybind11_abseil//pybind11_abseil:absl_casters",
        "@pybind11_abseil//pybind11_abseil:status_casters",
        "@pybind11_protobuf//pybind11_protobuf:wrapped_proto_caster",
    ],
)

cc_library(
    name = "results_testutils",
    hdrs = ["results_testutils.h"],
    deps = [
        "//meltan/lib/results",
        "//meltan/lib/results/internal:logging",
    ],
)

pytype_pybind_extension(
    name = "resultstestpy",
    srcs = ["resultstestpy.cc"],
    pytype_srcs = ["resultstestpy.pyi"],
    visibility = ["//visibility:public"],
    deps = [
        ":results_testutils",
        "@pybind11_protobuf//pybind11_protobuf:wrapped_proto_caster",
    ],
)
