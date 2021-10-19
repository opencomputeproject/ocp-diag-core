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

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

py_library(
    name = "testhelper",
    srcs = [
        "test.py",
        "testhelper.py",
        "teststep.py",
    ],
    deps = [
        "//meltan/lib/params:utils_py",
        "//meltan/lib/results:results_py_pb2",
        "//meltan/lib/results/python:results",
        "//third_party/pybind11_abseil:status",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_test(
    name = "testhelper_test",
    size = "small",
    srcs = ["testhelper_test.py"],
    python_version = "PY3",
    deps = [
        ":testhelper",
        "//meltan/lib/results:results_py_pb2",
        "//meltan/lib/results/python:results",
        "//third_party/pybind11_abseil:status",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_test(
    name = "teststep_test",
    size = "small",
    srcs = ["teststep_test.py"],
    python_version = "PY3",
    deps = [":testhelper"],
)
