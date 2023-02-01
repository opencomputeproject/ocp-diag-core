#!/bin/bash

set -eu

cd apis/c++
OUTPUT="$(bazel run //ocpdiag/core/examples/full_spec)"

cd ../../validators/spec_validator
echo "$OUTPUT" | go run . -schema ../../json_spec/output/root.json -
