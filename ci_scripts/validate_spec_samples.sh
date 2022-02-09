#!/bin/bash

set -eu

RESULTS_SCHEMA_PATH="$(readlink -f json_spec/results_spec.json)"

pushd validators/spec_validator
for jsonl in ./samples/*; do
    echo "Running sample $(basename $jsonl)"
    go run . -schema "$RESULTS_SCHEMA_PATH" "$jsonl"
done
