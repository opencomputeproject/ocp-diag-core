# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

#

load("//ocpdiag:python.bzl", "pytype_pybind_extension", "pytype_strict_library", pytype_strict_test = "pytype_strict_contrib_test")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

pytype_strict_library(
    name = "fake_params",
    testonly = 1,
    srcs = [
        "fake_params.py",
    ],
    data = [":_fake_params.so"],
    visibility = ["//visibility:public"],
    deps = [
        "//ocpdiag/core/results:results_py_pb2",
        "//third_party/pybind11_abseil:status",
        "@com_google_protobuf//:protobuf_python",
    ],
)

pytype_strict_test(
    name = "fake_params_test",
    timeout = "short",
    srcs = ["fake_params_test.py"],
    python_version = "PY3",
    visibility = ["//visibility:public"],
    deps = [
        ":fake_params",
        "//ocpdiag/core/params:utils_py",
        "//ocpdiag/core/params/testdata:test_params_py_pb2",
    ],
)

pytype_pybind_extension(
    name = "_fake_params",
    testonly = 1,
    srcs = ["fake_params.cc"],
    pytype_srcs = ["_fake_params.pyi"],
    deps = [
        "//ocpdiag/core/params:fake_params",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@pybind11_abseil//pybind11_abseil:absl_casters",
        "@pybind11_abseil//pybind11_abseil:status_casters",
        "@pybind11_protobuf//pybind11_protobuf:native_proto_caster",
    ],
)
