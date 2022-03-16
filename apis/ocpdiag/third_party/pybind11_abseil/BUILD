# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

# Python wrapper for pybind11_abseil status library.
licenses(["notice"])

py_library(
    name = "status",
    srcs = ["status.py"],
    data = ["@pybind11_abseil//pybind11_abseil:status.so"],
    visibility = ["//visibility:public"],
)
