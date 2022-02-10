# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
flags.DEFINE_boolean(
    'ocpdiag_copy_results_to_stdout',
    True,
    'Prints human-readable result artifacts to stdout in addition to default output',
    allow_hide_cpp=True)
flags.DEFINE_string(
    'ocpdiag_results_filepath',
    '',
    'Fully-qualified file path where binary-proto result data gets written.',
    allow_hide_cpp=True)
flags.DEFINE_string(
    'machine_under_test',
    'local',
    'Machine under test. If the test binary is running on the same machine as the machine under test, just keep the default \"local\".',
    allow_hide_cpp=True)
flags.DEFINE_boolean(
    'ocpdiag_strict_reporting',
    True,
    'Whether to require a global devpath to be reported in third_party.ocpdiag.results_pb.HardwareInfo',
    allow_hide_cpp=True)


# Register the flags to be set on init.
def _SetPybindFlags():
  SetResultsLibFlags(
      flags.FLAGS.ocpdiag_copy_results_to_stdout,
      flags.FLAGS.ocpdiag_results_filepath, flags.FLAGS.machine_under_test,
      flags.FLAGS.ocpdiag_strict_reporting)


app.call_after_init(_SetPybindFlags)
