// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef MELTAN_CORE_PARAMS_TESTDATA_DUMMY_OUTPUTS_H_
#define MELTAN_CORE_PARAMS_TESTDATA_DUMMY_OUTPUTS_H_

namespace ocpdiag {

constexpr const char* const kDummyBinaryArgsFileEnvVar =
    "MELTAN_DUMMY_TEST_ARG_DUMP_FILE";
constexpr const char* const kDummyBinaryStdinEnvVar =
    "MELTAN_DUMMY_TEST_STDIN_DUMP_FILE";

}  // namespace ocpdiag

#endif  // MELTAN_CORE_PARAMS_TESTDATA_DUMMY_OUTPUTS_H_
