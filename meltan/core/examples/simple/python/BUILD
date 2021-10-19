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

load("//meltan/core:meltan.bzl", "meltan_test_pkg")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

py_binary(
    name = "simple",
    srcs = ["simple.py"],
    python_version = "PY3",
    visibility = ["//visibility:public"],
    deps = [
        "//meltan/core/examples/simple:params_py_pb2",
        "//meltan/lib/params:utils_py",
        "//meltan/lib/results:results_py_pb2",
        "//meltan/lib/results/python:results",
        "//third_party/pybind11_abseil:status",
        "@com_google_absl_py//absl:app",
        "@com_google_protobuf//:protobuf_python",
    ],
)

meltan_test_pkg(
    name = "simple_py",
    binary = ":simple",
    json_defaults = "//meltan/core/examples/simple:params.json",
    params_proto = "//meltan/core/examples/simple:params_proto",
)
