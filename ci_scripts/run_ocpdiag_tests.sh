#!/bin/bash

set -eu

pushd apis/ocpdiag
bazel test ocpdiag/...
