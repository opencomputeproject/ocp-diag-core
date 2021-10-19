# Meltan



<!--*
freshness: { owner: 'yuanlinw' reviewed: '2021-09-23' }
*-->

Meltan is an open and multi-node friendly hardware diagnostics framework. The
framework provides a set of core libraries with a multi-language solution for
creating portable diagnostics that can integrate into a number of different test
platforms.

Meltan is *not* a
[test executive](https://en.wikipedia.org/wiki/Test_execution_engine). The
framework provides test executives with a single integration point for all
Meltan compatible diagnostics. The goal is to eliminate the wrapping effort
required in porting other diagnostics to different execution environments and
result schemas.

For diagnostics that run on multi-node systems, Meltan offers configuration
options for targeting different endpoints and backends.

## History

Google created the Meltan framework to improve integration with vendors during
the development of diagnostics for specific hardware. Meltan was shared with the
OCP group in August 2021 so that others could more easily ingress and
evaluate OCP designs and hardware.

Why this name? The Meltan team was inspired by the Pokémon
[`Meltan`](https://pokemonletsgo.pokemon.com/en-us/new-pokemon/) who can absorb
metal and generate electricity. Meltan can work individually or as a group, and
can evolve into the Melmetal, a stronger Pokémon who can attack even harder. The
Meltan team believes if a machine can survive Meltan diagnostics, it will be
able to survive real world data center stresses.

## Overview

Meltan diagnostics can run on a device under test (DUT) or off the DUT. After a
Meltan diag is built and installed, a test executive or a human needs to start
the test and collect its results.

![Meltan workflow](/meltan/g3doc/meltan_workflow.png)

Meltan focuses on three main topics: Parameter Model, Results Model, and
Hardware Interface.

This documentation provides schema specificaton, reference implementation, and
example usage for each of these topics. Each set of the core libraries can be
imported individually without affecting the usages of other subdomains.

The following paragraphs use a [simple example](meltan/core/examples/simple/)
to introduce the build rules, the models, and hardware interface.

## Running Meltan

A Meltan executable is a package that wraps a bash script with runfiles,
including:

*   Meltan parameter launcher
*   default parameters
*   parameter descriptor
*   test main binary and their dependencies

The GitHub repo contains a
`meltan_test_pkg` Bazel rule for building the Meltan target and a simple example
you can use to explore the build rules.

### Prerequisites

This workflow uses Bazel to build and test Meltan diagnostics. You must have
Bazel v4.0.0 or greater installed. See also Installing Bazel [instructions](https://docs.bazel.build/versions/main/install.html).


### Configuring the Bazel WORKSPACE

Add a WORKSPACE configuration file in the top-level repository where Bazel runs.
This enables Bazel to load Meltan core libraries.

```WORKSPACE
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "meltan",
    commit = "<put a commit hash here>",
    remote = "https://github.com/opencomputeproject/ocp-diag-core",
)

load("@meltan//meltan:build_deps.bzl", "load_deps")
load_deps()

load("@meltan//meltan:secondary_build_deps.bzl", "load_secondary_deps")
load_secondary_deps()

load("@meltan//meltan:tertiary_build_deps.bzl", "load_tertiary_deps")
load_tertiary_deps()
```



### Create a test BUILD file

In the BUILD file, define a binary, a parameter proto, and a default parameter
JSON file as the receipes to build the Meltan executable.

The following is an skeleton of the build receipts. For context, see the full
[BUILD file](/meltan/core/examples/simple/BUILD):

```BUILD
load("@meltan//meltan/core:meltan.bzl", "meltan_test_pkg")

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

# Meltan executable.
meltan_test_pkg(
    name = "simple",
    binary = ":simple_bin",
    json_defaults = "params.json",
    params_proto = ":params_proto",
)
```
### C++ Version Requirements

The Meltan project must be built with C++17. This can be accomplished by
adding `--cxxopt='-std=c++17'` to any bazel build command. It may be more
convenient to add a [`.bazelrc` file](https://docs.bazel.build/versions/main/guide.html#bazelrc-the-bazel-configuration-file) to the top level directory of your
project, with the following line:

```
build --cxxopt='-std=c++17'
```

which will apply the option to all build commands.

### Build the Meltan target.

The following command line builds the Meltan executable.

```shell
<your_terminal>$ bazel build meltan/core/examples/simple:simple --cxxopt='-std=c++17'
INFO: Build option --cxxopt has changed, discarding analysis cache.
INFO: Analyzed target //meltan/core/examples/simple:simple (62 packages loaded, 3568 targets configured).
INFO: Found 1 target...
Target //meltan/core/examples/simple:simple up-to-date:
  bazel-bin/meltan/core/examples/simple/simple
INFO: Elapsed time: 62.867s, Critical Path: 9.28s
INFO: 545 processes: 1 internal, 544 linux-sandbox.
INFO: Build completed successfully, 545 total actions
```

The executable appears under `./bazel-bin/meltan/core/examples/simple/simple` as indicated by bazel logs.

## Parameter model

The [Meltan Parameter](/meltan/g3doc/parameter.md) document defines
and describes how to use the Meltan parameter model.

## Results model

The [Meltan Results](/meltan/g3doc/results.md) document describes
the result objects, data schema, reference APIs, and best practices.

## Hardware interface

The [Meltan Hardware Interface](/meltan/g3doc/hardware_interface.md)
document describes how to define and use the Meltan Hardware Interface.

## Contact information

**Team:** meltan-core-team@google.com

**Code reviews:** meltan-core-team+reviews@google.com
