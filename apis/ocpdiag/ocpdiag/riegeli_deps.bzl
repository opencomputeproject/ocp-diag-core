# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

"""Contains a function for loading Riegeli dependencies."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def load_riegli_deps():
    maybe(
        http_archive,
        "highwayhash",
        urls = ["https://github.com/google/highwayhash/archive/c13d28517a4db259d738ea4886b1f00352a3cc33.zip"],  # 2022-04-06
        build_file = "@com_google_riegeli//third_party:highwayhash.BUILD",
        sha256 = "76f4c3cbb51bb111a0ba23756cf5eed099ea43fc5ffa7251faeed2a3c2d1adc0",
        strip_prefix = "highwayhash-c13d28517a4db259d738ea4886b1f00352a3cc33",
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
