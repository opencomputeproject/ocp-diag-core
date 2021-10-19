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

"""Load dependencies needed to compile the Meltan project.

Typical usage in a WORKSPACE file:
load("@meltan//meltan:build_deps.bzl", "load_deps")
load_deps()

load("@meltan//meltan:secondary_build_deps.bzl", "load_secondary_deps")
load_secondary_deps()

load("@meltan//meltan:tertiary_build_deps.bzl", "load_tertiary_deps")
load_tertiary_deps()
"""

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def load_deps(meltan_package_name = "meltan"):
    """Loads common dependencies needed to compile the protobuf library.

    Normally, users will pull in the Meltan package via

    git_repository(
          name = "meltan",
          ...
    )

    If another name is used, it needs to be forwarded here to resolve
    patches that need to be applied.

    Args:
      meltan_package_name: The name of the Meltan external package.
    """
    if not native.existing_rule("com_google_absl"):
        http_archive(
            name = "com_google_absl",
            sha256 = "11d6eea257cc9322cc49924cf9584dbe61922bfffe3e7c42e2bce3abc1694a1a",
            strip_prefix = "abseil-cpp-20210324.0",
            urls = ["https://github.com/abseil/abseil-cpp/archive/20210324.0.zip"],
        )

    if not native.existing_rule("com_google_absl_py"):
        http_archive(
            name = "com_google_absl_py",
            strip_prefix = "abseil-py-9954557f9df0b346a57ff82688438c55202d2188",
            urls = [
                "https://github.com/abseil/abseil-py/archive/9954557f9df0b346a57ff82688438c55202d2188.tar.gz",
            ],
        )

    if not native.existing_rule("six_archive"):
        # Six is named six_archive to match what absl_py expects.
        http_archive(
            name = "six_archive",
            urls = [
                "http://mirror.bazel.build/pypi.python.org/packages/source/s/six/six-1.10.0.tar.gz",
                "https://pypi.python.org/packages/source/s/six/six-1.10.0.tar.gz",
            ],
            sha256 = "105f8d68616f8248e24bf0e9372ef04d3cc10104f1980f54d57b2ce73a5ad56a",
            strip_prefix = "six-1.10.0",
            build_file = "@com_google_absl_py//third_party:six.BUILD",
        )

    if not native.existing_rule("com_google_googletest"):
        # Six is named six_archive to match what absl_py expects.
        http_archive(
            name = "com_google_googletest",
            sha256 = "9dc9157a9a1551ec7a7e43daea9a694a0bb5fb8bec81235d8a1e6ef64c716dcb",
            strip_prefix = "googletest-release-1.10.0",
            urls = ["https://github.com/google/googletest/archive/release-1.10.0.tar.gz"],
        )

    if not native.existing_rule("com_google_protobuf"):
        # Protocol buffers. Official release 3.13.0.1
        patch_path = "@{}//external:com_google_protobuf_build.patch".format(meltan_package_name)
        git_repository(
            name = "com_google_protobuf",
            remote = "https://github.com/protocolbuffers/protobuf.git",
            tag = "v3.18.0-rc1",
            patches = [patch_path],
        )

    if not native.existing_rule("com_github_grpc_grpc"):
        http_archive(
            name = "com_github_grpc_grpc",
            sha256 = "abd9e52c69000f2c051761cfa1f12d52d8b7647b6c66828a91d462e796f2aede",
            urls = [
                "https://github.com/grpc/grpc/archive/v1.38.0.tar.gz",
            ],
            strip_prefix = "grpc-1.38.0",
        )

    if not native.existing_rule("pybind11_bazel"):
        http_archive(
            name = "pybind11_bazel",
            strip_prefix = "pybind11_bazel-26973c0ff320cb4b39e45bc3e4297b82bc3a6c09",
            urls = ["https://github.com/pybind/pybind11_bazel/archive/26973c0ff320cb4b39e45bc3e4297b82bc3a6c09.zip"],
        )

    if not native.existing_rule("pybind11"):
        http_archive(
            name = "pybind11",
            sha256 = "4828cd4b5b637ac5bc1106655931e5fe8dd6b56db6c24ccf7b44d3b2c55f49c2",
            build_file = "@pybind11_bazel//:pybind11.BUILD",
            strip_prefix = "pybind11-0c93a0f3fcf6bf26be584558d7426564720cea6f",
            urls = ["https://github.com/pybind/pybind11/archive/0c93a0f3fcf6bf26be584558d7426564720cea6f.tar.gz"],
        )

    if not native.existing_rule("pybind11_abseil"):
        git_repository(
            name = "pybind11_abseil",
            commit = "7cf76381300671ade35127cb25566d588c69e717",
            remote = "https://github.com/pybind/pybind11_abseil.git",
        )

    if not native.existing_rule("pybind11_protobuf"):
        git_repository(
            name = "pybind11_protobuf",
            # commit = "3f4272fa6da5417a93a4fff8474439e6f12e95e8",
            branch = "main",
            remote = "https://github.com/pybind/pybind11_protobuf.git",
        )

    if not native.existing_rule("rules_python"):
        http_archive(
            name = "rules_python",
            url = "https://github.com/bazelbuild/rules_python/releases/download/0.2.0/rules_python-0.2.0.tar.gz",
            sha256 = "778197e26c5fbeb07ac2a2c5ae405b30f6cb7ad1f5510ea6fdac03bded96cc6f",
        )

    if not native.existing_rule("rules_pkg"):
        http_archive(
            name = "rules_pkg",
            urls = [
                "https://mirror.bazel.build/github.com/bazelbuild/rules_pkg/releases/download/0.5.0/rules_pkg-0.5.0.tar.gz",
                "https://github.com/bazelbuild/rules_pkg/releases/download/0.5.0/rules_pkg-0.5.0.tar.gz",
            ],
            sha256 = "353b20e8b093d42dd16889c7f918750fb8701c485ac6cceb69a5236500507c27",
        )

    if not native.existing_rule("com_google_ecclesia"):
        git_repository(
            name = "com_google_ecclesia",
            branch = "master",
            remote = "https://github.com/google/ecclesia-machine-management.git",
        )
