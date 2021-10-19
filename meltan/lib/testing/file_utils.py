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

"""File-finding utilities."""
import os

_GOOGLE_WORKSPACE = 'google3'


def data_file_path_prefix():
  """Retrieves the root path for all data dependencies.

  In bazel, the TEST_WORKSPACE variable is appended to the SRCDIR, whereas
  in blaze it's ignored.

  Returns:
    The root path string to the data dependency tree.
  """
  test_srcdir = os.environ.get('TEST_SRCDIR')
  test_workspace = os.environ.get('TEST_WORKSPACE')
  if test_workspace == _GOOGLE_WORKSPACE:
    return test_srcdir
  return '{:s}/{:s}'.format(test_srcdir, test_workspace)
