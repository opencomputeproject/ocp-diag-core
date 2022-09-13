# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

"""Load dependencies needed to compile the OCPDiag project.

Typical usage in a WORKSPACE file:
load("@ocpdiag//ocpdiag:build_deps.bzl", "load_deps")
load_deps()

load("@ocpdiag//ocpdiag:secondary_build_deps.bzl", "load_secondary_deps")
load_secondary_deps()

load("@ocpdiag//ocpdiag:tertiary_build_deps.bzl", "load_tertiary_deps")
load_tertiary_deps()
"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def load_deps(ocpdiag_package_name = "ocpdiag"):
    """Loads common dependencies needed to compile the protobuf library.

    Normally, users will pull in the OCPDiag package via

    http_archive(
        name = "ocpdiag",
        ...
    )

    If another name is used, it needs to be forwarded here to resolve
    patches that need to be applied to imported projects.

    In general, dependencies should point to http archives rather than
    git repositories and should be pinned to a particular version if
    it exists, or a particular commit if the repo doesn't have versions.
    The sha256 hash should be included for each rule as well to prevent
    accidental dependency changes. See the abseil live at head policy
    here: https://github.com/abseil/abseil-cpp/blob/master/FAQ.md#what-is-live-at-head-and-how-do-i-do-it

    Args:
      ocpdiag_package_name: The name of the OCPDiag external package.
    """
    maybe(
        http_archive,
        "com_google_absl",
        url = "https://github.com/abseil/abseil-cpp/archive/92fdbfb301f8b301b28ab5c99e7361e775c2fb8a.zip",  # 2022-08-25
        strip_prefix = "abseil-cpp-92fdbfb301f8b301b28ab5c99e7361e775c2fb8a",
        sha256 = "feea6e761ab9df667f054531b4e4d5d143eaa78f2c40dbf5ba45289142cbcc1d",
    )

    maybe(
        http_archive,
        "com_google_absl_py",
        url = "https://github.com/abseil/abseil-py/archive/f0679ed8e79d1352f23b80965981b704bd48e1a4.zip",  # 2022-07-18
        strip_prefix = "abseil-py-f0679ed8e79d1352f23b80965981b704bd48e1a4",
        sha256 = "35b506dd7a3da7f1a176191f0486eb990b71f6d8b685da698281d9abb995accc",
    )

    maybe(
        http_archive,
        "com_google_googletest",
        url = "https://github.com/google/googletest/archive/61720589cbef9707b3c5247d02141a7c31e2876f.zip",  # 2022-07-20
        strip_prefix = "googletest-61720589cbef9707b3c5247d02141a7c31e2876f",
        sha256 = "bb6329cfb96f688d3e62f2077a751f250e046f0996137b1b310a2fbcaab10198",
    )

    # Six is named six_archive to match what absl_py expects.
    build_path = "@{}//external:six.BUILD".format(ocpdiag_package_name)
    maybe(
        http_archive,
        "six_archive",
        urls = [
            "http://mirror.bazel.build/pypi.python.org/packages/source/s/six/six-1.16.0.tar.gz",
            "https://pypi.python.org/packages/source/s/six/six-1.16.0.tar.gz",
        ],
        strip_prefix = "six-1.16.0",
        sha256 = "1e61c37477a1626458e36f7b1d82aa5c9b094fa4802892072e49de9c60c4c926",
        build_file = build_path,
    )

    maybe(
        http_archive,
        "com_google_protobuf",
        url = "https://github.com/protocolbuffers/protobuf/releases/download/v21.5/protobuf-all-21.5.tar.gz",
        strip_prefix = "protobuf-21.5",
        sha256 = "7ba0cb2ecfd9e5d44a6fa9ce05f254b7e5cd70ec89fafba0b07448f3e258310c",
    )

    maybe(
        http_archive,
        "com_github_grpc_grpc",
        url = "https://github.com/grpc/grpc/archive/v1.48.0.tar.gz",
        strip_prefix = "grpc-1.48.0",
        sha256 = "9b1f348b15a7637f5191e4e673194549384f2eccf01fcef7cc1515864d71b424",
    )

    patch_path = "@{}//external:pybind11_python_config.patch".format(ocpdiag_package_name)
    maybe(
        http_archive,
        "pybind11_bazel",
        url = "https://github.com/pybind/pybind11_bazel/archive/72cbbf1fbc830e487e3012862b7b720001b70672.zip",  # 2021-12-03
        strip_prefix = "pybind11_bazel-72cbbf1fbc830e487e3012862b7b720001b70672",
        sha256 = "fec6281e4109115c5157ca720b8fe20c8f655f773172290b03f57353c11869c2",
        patches = [patch_path],
    )

    maybe(
        http_archive,
        "pybind11",
        url = "https://github.com/pybind/pybind11/archive/refs/tags/v2.10.0.tar.gz",
        strip_prefix = "pybind11-2.10.0",
        sha256 = "eacf582fa8f696227988d08cfc46121770823839fe9e301a20fbce67e7cd70ec",
        build_file = "@pybind11_bazel//:pybind11.BUILD",
    )

    maybe(
        http_archive,
        "pybind11_abseil",
        url = "https://github.com/pybind/pybind11_abseil/archive/0c45d0f5c804cd30bf59013ebcc09f6c1460bd87.zip",  # 2022-09-08
        strip_prefix = "pybind11_abseil-0c45d0f5c804cd30bf59013ebcc09f6c1460bd87",
        sha256 = "3df6de84ddfc9331c4d2a1a1c1e05283ac97176e596ba73dbb5cc7aa72756695",
    )

    maybe(
        http_archive,
        "pybind11_protobuf",
        url = "https://github.com/pybind/pybind11_protobuf/archive/9750a61a0a0e6978c5fa91b764b1e4886e9009cc.zip",  # 2022-08-09
        strip_prefix = "pybind11_protobuf-9750a61a0a0e6978c5fa91b764b1e4886e9009cc",
        sha256 = "352848326837413caf25fb0bb3ae94430c6893c594f05cdaf4860201182f0fb8",
    )

    maybe(
        http_archive,
        "rules_python",
        url = "https://github.com/bazelbuild/rules_python/archive/refs/tags/0.10.2.tar.gz",
        strip_prefix = "rules_python-0.10.2",
        sha256 = "a3a6e99f497be089f81ec082882e40246bfd435f52f4e82f37e89449b04573f6",
    )

    maybe(
        http_archive,
        "rules_pkg",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/rules_pkg/releases/download/0.7.0/rules_pkg-0.7.0.tar.gz",
            "https://github.com/bazelbuild/rules_pkg/releases/download/0.7.0/rules_pkg-0.7.0.tar.gz",
        ],
        sha256 = "8a298e832762eda1830597d64fe7db58178aa84cd5926d76d5b744d6558941c2",
    )

    maybe(
        http_archive,
        "com_google_ecclesia",
        url = "https://github.com/google/ecclesia-machine-management/archive/92ae4d367c7a3a892bf672cdca424b75677c2bb2.zip",  # 2022-08-10
        strip_prefix = "ecclesia-machine-management-92ae4d367c7a3a892bf672cdca424b75677c2bb2",
        sha256 = "92d1acae14262951e0b0c8c7c7c34996353548921008d3e6f59f2d0d0ec123c3",
    )
