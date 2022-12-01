# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

# These packages validate OCPDiag diagnostic results.
#
# See go/ocpdiag-test-validation-framework-design for more info.

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "unit_test_validator",
    testonly = 1,
    srcs = ["unit_test_validator.cc"],
    hdrs = ["unit_test_validator.h"],
    visibility = [
        # Restrict visibility on a need-to-know basis. This package should not be needed widely.
        "//ocpdiag/diagnostics/lib/test_harness:__pkg__",
    ],
    deps = [
        "//ocpdiag/core/compat:status_macros",
        "//ocpdiag/core/results:recordio_iterator",
        "//ocpdiag/core/results:results_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_googlesource_code_re2//:re2",
    ],
)

cc_test(
    name = "unit_test_validator_test",
    srcs = [
        "unit_test_validator_test.cc",
    ],
    deps = [
        ":unit_test_validator",
        "//ocpdiag/core/results:results_cc_proto",
        "//ocpdiag/core/testing:file_utils",
        "//ocpdiag/core/testing:status_matchers",
        "@com_google_googletest//:gtest_main",
        "@com_google_riegeli//riegeli/bytes:fd_writer",
        "@com_google_riegeli//riegeli/records:record_writer",
    ],
)
