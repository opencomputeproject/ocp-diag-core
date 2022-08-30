# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

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
