# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

load("//ocpdiag:packaging.bzl", "ocpdiag_test_pkg")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

py_binary(
    name = "simple",
    srcs = ["simple.py"],
    python_version = "PY3",
    visibility = ["//visibility:public"],
    deps = [
        "//ocpdiag/core/examples/simple:params_py_pb2",
        "//ocpdiag/core/hwinterface:cpu_py_pb2",
        "//ocpdiag/core/hwinterface:service_py_pb2",
        "//ocpdiag/core/hwinterface/client/python:node_client",
        "//ocpdiag/core/params:utils_py",
        "//ocpdiag/core/results:results_py_pb2",
        "//ocpdiag/core/results/python:results",
        "//third_party/pybind11_abseil:status",
        "@com_google_absl_py//absl:app",
        "@com_google_protobuf//:protobuf_python",
    ],
)

ocpdiag_test_pkg(
    name = "simple_py",
    binary = ":simple",
    json_defaults = "//ocpdiag/core/examples/simple:params.json",
    params_proto = "//ocpdiag/core/examples/simple:params_proto",
)
