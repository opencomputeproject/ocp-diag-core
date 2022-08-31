# OCPDiag



<!--*
freshness: { owner: 'yuanlinw' reviewed: '2022-03-22' }
*-->

OCPDiag is an open and multi-node friendly hardware diagnostics framework. The
framework provides a set of core libraries with a multi-language solution for
creating portable diagnostics that can integrate into a number of different test
platforms. Random Change to Trigger Github push!

OCPDiag is *not* a
[test executive](https://en.wikipedia.org/wiki/Test_execution_engine). The
framework provides test executives with a single integration point for all
OCPDiag compatible diagnostics. The goal is to eliminate the wrapping effort
required in porting other diagnostics to different execution environments and
result schemas.

For diagnostics that run on multi-node systems, OCPDiag offers configuration
options for targeting different endpoints and backends.

## Overview

OCPDiag diagnostics can run on a device under test (DUT) or off the DUT. After a
OCPDiag diag is built and installed, a test executive or a human needs to start
the test and collect its results.

![OCPDiag workflow](/ocpdiag/g3doc/ocpdiag_workflow.png)

OCPDiag focuses on three main topics: Parameter Model, Results Model, and
Hardware Interface.

This documentation provides schema specificaton, reference implementation, and
example usage for each of these topics. Each set of the core libraries can be
imported individually without affecting the usages of other subdomains.

The following paragraphs use a [simple example](ocpdiag/core/examples/simple/)
to introduce the build rules, the models, and hardware interface.

## Running OCPDiag

A OCPDiag executable is a package that wraps a bash script with runfiles,
including:

*   OCPDiag parameter launcher
*   default parameters
*   parameter descriptor
*   test main binary and their dependencies

The GitHub repo contains a
`ocpdiag_test_pkg` Bazel rule for building the OCPDiag target and a simple example
you can use to explore the build rules.

### Prerequisites

This workflow uses Bazel to build and test OCPDiag diagnostics. You must have
Bazel v4.0.0 or greater installed. See also Installing Bazel [instructions](https://docs.bazel.build/versions/main/install.html).


### Configuring the Bazel WORKSPACE

Add a WORKSPACE configuration file in the top-level repository where Bazel runs.
This enables Bazel to load OCPDiag core libraries.

```WORKSPACE
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "ocpdiag",
    commit = <pick a commit hash, put here in quotes>,
    remote = "https://github.com/opencomputeproject/ocp-diag-core",
)

load("@ocpdiag//ocpdiag:build_deps.bzl", "load_deps")
load_deps()

load("@ocpdiag//ocpdiag:secondary_build_deps.bzl", "load_secondary_deps")
load_secondary_deps()

load("@ocpdiag//ocpdiag:tertiary_build_deps.bzl", "load_tertiary_deps")
load_tertiary_deps()
```



### Create a test BUILD file

In the BUILD file, define a binary, a parameter proto, and a default parameter
JSON file as the receipes to build the OCPDiag executable.

The following is an skeleton of the build receipts. For context, see the full
[BUILD file](/ocpdiag/core/examples/simple/BUILD):

```BUILD
load("@ocpdiag//ocpdiag/core:ocpdiag.bzl", "ocpdiag_test_pkg")

# Proto parameter library.
proto_library(
    name = "params_proto",
    srcs = ["params.proto"],
)

cc_proto_library(
    name = "params_cc_proto",
    deps = [":params_proto"],
)

# Test binary
cc_binary(
    name = "simple_bin",
    srcs = ["simple_main.cc"],
    deps = [
        ":params_cc_proto",
        # Other dependencies.
    ],
)

# OCPDiag executable.
ocpdiag_test_pkg(
    name = "simple",
    binary = ":simple_bin",
    json_defaults = "params.json",
    params_proto = ":params_proto",
)
```
### C++ Version Requirements

The OCPDiag project must be built with C++17. This can be accomplished by
adding `--cxxopt='-std=c++17'` to any bazel build command. It may be more
convenient to add a [`.bazelrc` file](https://docs.bazel.build/versions/main/guide.html#bazelrc-the-bazel-configuration-file) to the top level directory of your
project, with the following line:

```
build --cxxopt='-std=c++17'
```

which will apply the option to all build commands.

### Build the OCPDiag target.

The following command line builds the OCPDiag executable.

```shell
<your_terminal>$ bazel build ocpdiag/core/examples/simple:simple --cxxopt='-std=c++17'
INFO: Build option --cxxopt has changed, discarding analysis cache.
INFO: Analyzed target //ocpdiag/core/examples/simple:simple (62 packages loaded, 3568 targets configured).
INFO: Found 1 target...
Target //ocpdiag/core/examples/simple:simple up-to-date:
  bazel-bin/ocpdiag/core/examples/simple/simple
INFO: Elapsed time: 62.867s, Critical Path: 9.28s
INFO: 545 processes: 1 internal, 544 linux-sandbox.
INFO: Build completed successfully, 545 total actions
```

The executable appears under `./bazel-bin/ocpdiag/core/examples/simple/simple` as indicated by bazel logs.

**NOTE**: OCPDiag executable is an compressed tarball embedded in a shell script.
During runtime, the tarball will be extracted to `TMPDIR`. If `TMPDIR` is not
defined, a default `/tmp` folder will be used. If you are sensitive to temp
memory usage, please override `TMPDIR` to another folder which allows execution.
The following is an example where the tarball is extracted to `/data/` rather
than the default `/tmp/`:

```
$ TMPDIR=/data ./simple --dry_run | head -n1
This test was started with --dry_run.If it was actually run, the raw arguments would have been
/data/simple.MaN/_ocpdiag_test_pkg_simple_launcher.runfiles/google3/third_party/ocpdiag/core/examples/simple/simple_bin --dry_run
It would be passed the parameters via stdin.
{
```

## Parameter model

The [OCPDiag Parameter](/ocpdiag/g3doc/parameter.md) document defines
and describes how to use the OCPDiag parameter model.

## Results model

The [OCPDiag Results](/ocpdiag/g3doc/results.md) document describes
the result objects, data schema, reference APIs, and best practices.

## Hardware interface

The [OCPDiag Hardware Interface](/ocpdiag/g3doc/hardware_interface.md)
document describes how to define and use the OCPDiag Hardware Interface.

## Contact information

**Team:** ocp-test-validation@OCP-All.groups.io

**Code reviews:** ocpdiag-core-team+reviews@google.com
