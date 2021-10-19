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

# Noop is a minimal no-op meltan test.

load("//meltan/core:meltan.bzl", "meltan_test_pkg")

licenses(["notice"])

cc_binary(
    name = "noop_bin",
    srcs = ["noop.cc"],
)

meltan_test_pkg(
    name = "noop",
    binary = ":noop_bin",
)

meltan_test_pkg(
    name = "noop_defaults",
    binary = ":noop_bin",
    json_defaults = "empty.json",
)

py_test(
    name = "noop_test",
    srcs = ["noop_test.py"],
    data = [
        ":noop",
        ":noop_defaults",
    ],
    python_version = "PY3",
    tags = ["nozapfhahn"],  # copybara_strip(internal coverage metric tag)
    deps = ["//meltan/lib/testing:file_utils_py"],
)
