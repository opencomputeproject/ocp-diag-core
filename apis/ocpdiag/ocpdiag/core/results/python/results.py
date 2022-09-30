# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

"""Wrapper for the ocpdiag results pybind library.

This wrapper is used to set the proper C++ flags for the ocpdiag results library.
In google3, this isn't necessary but in open source abseil-py lacks python/C++
interop.

As a bonus, this gives a python library import for any python users, rather
than needing to worry about directly importing the pybind_extension (which is
a data dependency in open source).
"""

from ._results import *  # pylint:disable=wildcard-import
from ._results import SetResultsLibFlags
from absl import app
from absl import flags

# Define the flags with identical names to corresponding c++ flags.
_OCPDIAG_COPY_RESULTS_TO_STDOUT = flags.DEFINE_boolean(
    'ocpdiag_copy_results_to_stdout',
    True,
    'Prints human-readable result artifacts to stdout in addition to default '
    'output',
    allow_hide_cpp=True)
_OCPDIAG_RESULTS_FILEPATH = flags.DEFINE_string(
    'ocpdiag_results_filepath',
    '',
    'Fully-qualified file path where binary-proto result data gets written.',
    allow_hide_cpp=True)
_MACHINE_UNDER_TEST = flags.DEFINE_string(
    'machine_under_test',
    'local',
    'Machine under test. If the test binary is running on the same machine as '
    'the machine under test, just keep the default \"local\".',
    allow_hide_cpp=True)
_OCPDIAG_STRICT_REPORTING = flags.DEFINE_boolean(
    'ocpdiag_strict_reporting',
    True, 'Whether to require a global devpath to be reported in '
    'third_party.ocpdiag.results_pb.HardwareInfo',
    allow_hide_cpp=True)
_ALSOLOGTOOCPDIAGRESULTS = flags.DEFINE_boolean(
    'alsologtoocpdiagresults',
    False,
    'If set to true, ABSL and Ecclesia logger will be also direct to OCPDiag '
    'results other than ABSL default logging destination.',
    allow_hide_cpp=True)


# Register the flags to be set on init.
def _SetPybindFlags():
  SetResultsLibFlags(
      _OCPDIAG_COPY_RESULTS_TO_STDOUT.value,
      _OCPDIAG_RESULTS_FILEPATH.value,
      _MACHINE_UNDER_TEST.value,
      _ALSOLOGTOOCPDIAGRESULTS.value,
      _OCPDIAG_STRICT_REPORTING.value,
  )


app.call_after_init(_SetPybindFlags)
