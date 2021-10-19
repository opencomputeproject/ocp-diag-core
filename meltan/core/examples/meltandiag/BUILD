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

cc_binary(
    name = "my_meltan_diagnostic_bin",
    srcs = ["main.cc"],
    deps = [
        ":my_meltan_diagnostic",
        "//meltan/lib/results",
        "@com_google_absl//absl/flags:parse",
    ],
)

cc_test(
    name = "my_meltan_diagnostic_test",
    srcs = ["my_meltan_diagnostic_test.cc"],
    deps = [
        ":my_meltan_diagnostic",
        "//meltan/lib/results",
        "//meltan/lib/results:results_cc_proto",
        "//meltan/lib/testing:mock_results",
        "//meltan/lib/testing:proto_matchers",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/types:span",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "my_meltan_diagnostic",
    srcs = ["my_meltan_diagnostic.cc"],
    hdrs = ["my_meltan_diagnostic.h"],
    deps = [
        "//meltan/lib:meltan_diagnostic_lib",
        "//meltan/lib/results",
        "//meltan/lib/results:results_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/types:span",
    ],
)

proto_library(
    name = "params_proto",
    srcs = ["params.proto"],
)

cc_proto_library(
    name = "params_cc_proto",
    deps = [":params_proto"],
)
