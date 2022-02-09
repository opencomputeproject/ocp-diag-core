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

"""Provides parameter-parsing utilities for python ocpdiag tests."""

import os
import sys
from google.protobuf import json_format
from google.protobuf import message


def GetParams(params_msg: message.Message):
  """GetParams retrieves the stdin ocpdiag params into the passed proto message.

  Args:
    params_msg: Protobuf message prototype.
  Returns:
    Instance of passed message type with populated fields.
  """
  # Don't support interactive input.
  if sys.stdin.isatty() and not os.getenv('OCPDIAG_STDIN'):
    return params_msg
  return json_format.Parse(sys.stdin.read(), params_msg)
