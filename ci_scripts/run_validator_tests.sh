#!/bin/bash

set -eu

pushd validators/spec_validator
go test -race ./...
