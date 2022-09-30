# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

"""Wrapper for ocpdiag results output model."""
from ocpdiag.core.results.python import _output_model

MeasurementSeriesOutput = _output_model.MeasurementSeriesOutput
TestStepOutput = _output_model.TestStepOutput
TestRunOutput = _output_model.TestRunOutput
