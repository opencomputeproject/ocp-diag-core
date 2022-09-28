# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

"""Load and configure transitive dependencies.

These are custom dependency loading functions provided by external
projects - due to bazel load formatting these cannot be loaded
in "build_deps.bzl".
"""

load("@pybind11_bazel//:python_configure.bzl", "python_configure")
load("@rules_pkg//:deps.bzl", "rules_pkg_dependencies")
load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")
load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")
load("@com_google_ecclesia//ecclesia/build_defs:deps_first.bzl", "ecclesia_deps_first")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def load_secondary_deps():
    """Loads transitive dependencies of projects imported from build_deps.bzl"""
    grpc_deps()
    protobuf_deps()
    rules_pkg_dependencies()
    ecclesia_deps_first()
    python_configure(name = "local_config_python", python_version = "3")

    # Riegeli dependencies ------------------------------------------------------
    maybe(
        http_archive,
        "highwayhash",
        urls = ["https://github.com/google/highwayhash/archive/276dd7b4b6d330e4734b756e97ccfb1b69cc2e12.zip"],  # 2019-02-22
        build_file = "@com_google_riegeli//third_party:highwayhash.BUILD",
        sha256 = "cf891e024699c82aabce528a024adbe16e529f2b4e57f954455e0bf53efae585",
        strip_prefix = "highwayhash-276dd7b4b6d330e4734b756e97ccfb1b69cc2e12",
    )

    maybe(
        http_archive,
        "org_brotli",
        urls = ["https://github.com/google/brotli/archive/68f1b90ad0d204907beb58304d0bd06391001a4d.zip"],  # 2021-08-18
        sha256 = "fec5a1d26f3dd102c542548aaa704f655fecec3622a24ec6e97768dcb3c235ff",
        strip_prefix = "brotli-68f1b90ad0d204907beb58304d0bd06391001a4d",
    )

    maybe(
        http_archive,
        "net_zstd",
        urls = ["https://github.com/facebook/zstd/archive/v1.4.5.zip"],  # 2020-05-22
        build_file = "@com_google_riegeli//third_party:net_zstd.BUILD",
        sha256 = "b6c537b53356a3af3ca3e621457751fa9a6ba96daf3aebb3526ae0f610863532",
        strip_prefix = "zstd-1.4.5/lib",
    )

    maybe(
        http_archive,
        "snappy",
        urls = ["https://github.com/google/snappy/archive/1.1.8.zip"],  # 2020-01-14
        strip_prefix = "snappy-1.1.8",
        build_file = "@com_google_riegeli//third_party:snappy.BUILD",
        sha256 = "38b4aabf88eb480131ed45bfb89c19ca3e2a62daeb081bdf001cfb17ec4cd303",
    )
