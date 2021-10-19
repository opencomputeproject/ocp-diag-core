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

# Example monitors for infra smoke test.
load("//meltan/core:meltan.bzl", "meltan_test_pkg")

licenses(["notice"])

proto_library(
    name = "params_proto",
    srcs = ["params.proto"],
)

cc_proto_library(
    name = "params_cc_proto",
    deps = [":params_proto"],
)

cc_binary(
    name = "monitor_bin",
    srcs = ["main.cc"],
    deps = [
        ":params_cc_proto",
        "//meltan/lib/params:utils",
        "//meltan/lib/results",
        "//meltan/lib/results:results_cc_proto",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
    ],
)

meltan_test_pkg(
    name = "example_monitor",
    binary = ":monitor_bin",
    json_defaults = "params.json",
    params_proto = ":params_proto",
)
