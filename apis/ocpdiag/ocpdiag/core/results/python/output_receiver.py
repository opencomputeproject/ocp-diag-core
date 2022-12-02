# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

"""Wrapper for the ocpdiag output receiver pybind library."""

from ocpdiag.core.results.python import _output_receiver
from ocpdiag.core.results.python import results  # pylint:disable=unused-import

OutputReceiver = _output_receiver.OutputReceiver
