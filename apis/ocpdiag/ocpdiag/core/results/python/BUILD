# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

# Description:
#   BUILD file for the clif example illustrating wrapping inheritance

load("//ocpdiag:python.bzl", "pytype_pybind_extension", "pytype_strict_library")
load("//ocpdiag:python.bzl", "py_strict_test")

licenses(["notice"])

pytype_strict_library(
    name = "results",
    srcs = ["results.py"],
    data = [":_results.so"],
    visibility = ["//visibility:public"],
    deps = [
        ":output_model",
        "//ocpdiag/core/results:results_py_pb2",  # buildcleaner: keep
        "@com_google_absl_py//absl:app",
        "@com_google_absl_py//absl/flags",
        "@com_google_protobuf//:protobuf_python",  # buildcleaner: keep
    ],
)

pytype_pybind_extension(
    name = "_results",
    srcs = ["results.cc"],
    pytype_srcs = ["_results.pyi"],
    visibility = ["//ocpdiag/diagnostics/lib/test_harness/python:__subpackages__"],
    deps = [
        "//ocpdiag/core/results",
        "//ocpdiag/core/results:output_receiver",
        "//ocpdiag/core/results:results_cc_proto",
        "@com_google_absl//absl/flags:flag",
        "@pybind11_abseil//pybind11_abseil:absl_casters",
        "@pybind11_abseil//pybind11_abseil:status_casters",
        "@pybind11_protobuf//pybind11_protobuf:native_proto_caster",
        "@pybind11_protobuf//pybind11_protobuf:wrapped_proto_caster",
    ],
)

cc_library(
    name = "results_testutils",
    hdrs = ["results_testutils.h"],
    deps = [
        "//ocpdiag/core/results",
        "//ocpdiag/core/results/internal:logging",
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

pytype_strict_library(
    name = "output_model",
    srcs = ["output_model.py"],
    data = [":_output_model.so"],
    visibility = ["//visibility:public"],
    deps = [
        "//ocpdiag/core/results:results_py_pb2",  # buildcleaner: keep
        "@com_google_absl_py//absl:app",
        "@com_google_absl_py//absl/flags",
        "@com_google_protobuf//:protobuf_python",  # buildcleaner: keep
    ],
)

pytype_pybind_extension(
    name = "_output_model",
    srcs = ["output_model.cc"],
    pytype_srcs = ["_output_model.pyi"],
    deps = [
        "//ocpdiag/core/results:output_model",
        "@pybind11_abseil//pybind11_abseil:absl_casters",
        "@pybind11_abseil//pybind11_abseil:status_casters",
        "@pybind11_protobuf//pybind11_protobuf:native_proto_caster",
    ],
)

py_strict_test(
    name = "output_model_test",
    timeout = "short",
    srcs = ["output_model_test.py"],
    python_version = "PY3",
    visibility = ["//visibility:public"],
    deps = [
        ":output_model",
        "//ocpdiag/core/results:results_py_pb2",
        "@com_google_absl_py//absl:app",
        "@com_google_absl_py//absl/flags",
    ],
)
