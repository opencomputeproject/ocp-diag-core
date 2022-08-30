# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

# Noop is a minimal no-op ocpdiag test.

load("//ocpdiag:packaging.bzl", "ocpdiag_test_pkg")

licenses(["notice"])

cc_binary(
    name = "noop_bin",
    srcs = ["noop.cc"],
)

ocpdiag_test_pkg(
    name = "noop",
    binary = ":noop_bin",
)

ocpdiag_test_pkg(
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
    deps = ["//ocpdiag/core/testing:file_utils_py"],
)
