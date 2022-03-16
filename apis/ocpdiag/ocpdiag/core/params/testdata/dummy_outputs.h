// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_PARAMS_TESTDATA_DUMMY_OUTPUTS_H_
#define OCPDIAG_CORE_PARAMS_TESTDATA_DUMMY_OUTPUTS_H_

namespace ocpdiag {

constexpr const char* const kDummyBinaryArgsFileEnvVar =
    "OCPDIAG_DUMMY_TEST_ARG_DUMP_FILE";
constexpr const char* const kDummyBinaryStdinEnvVar =
    "OCPDIAG_DUMMY_TEST_STDIN_DUMP_FILE";

}  // namespace ocpdiag

#endif  // OCPDIAG_CORE_PARAMS_TESTDATA_DUMMY_OUTPUTS_H_
