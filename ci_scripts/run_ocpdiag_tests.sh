#!/bin/bash

set -eu
cd apis/c++
bazel build ...
bazel test ... --test_output=errors
