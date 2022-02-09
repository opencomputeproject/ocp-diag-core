#!/bin/bash

set -eu

if false; then
# disabled due to current inconsistencies between proto file and json spec

for e in simple simple/python simple_failure; do
    pushd apis/ocpdiag
    OUTPUT="$(bazel run ocpdiag/core/examples/$e)"
    popd

    pushd validators/spec_validator
    echo "$OUTPUT" | go run . -
    popd
done
fi