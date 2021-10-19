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
    name = "binary_task_base",
    srcs = ["binary_task_base.py"],
    srcs_version = "PY3",
    deps = ["//meltan/lib/results/python:results"],
)

py_test(
    name = "binary_task_base_test",
    srcs = [
        "binary_task_base_test.py",
    ],
    python_version = "PY3",
    deps = [
        ":binary_task_base",
        "//meltan/lib/results/python:results",
        "@com_google_absl_py//absl/testing:absltest",
        "@com_google_protobuf//:protobuf_python",
    ],
)
