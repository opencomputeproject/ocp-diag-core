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

"""Load and configure transitive dependencies.

These are custom dependency loading functions provided by external
projects - due to bazel load formatting these cannot be loaded
in "build_deps.bzl".
"""

load("@pybind11_bazel//:python_configure.bzl", "python_configure")
load("@rules_pkg//:deps.bzl", "rules_pkg_dependencies")
load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")
load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")

def load_secondary_deps():
    """Loads transitive dependencies of projects imported from build_deps.bzl"""
    grpc_deps()
    protobuf_deps()
    rules_pkg_dependencies()
    python_configure(name = "local_config_python", python_version = "3")
