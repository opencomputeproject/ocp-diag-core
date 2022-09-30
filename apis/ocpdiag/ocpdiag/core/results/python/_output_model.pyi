# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

from typing import Dict, List, Optional

from ocpdiag.core.results import results_pb2


class MeasurementSeriesOutput:
  start: Optional[results_pb2.MeasurementSeriesStart] = ...
  end: Optional[results_pb2.MeasurementSeriesEnd] = ...
  measurement_elements: List[results_pb2.MeasurementElement] = ...


class TestStepOutput:
  start: Optional[results_pb2.TestStepStart] = ...
  end: Optional[results_pb2.TestStepEnd] = ...
  logs: Dict[results_pb2.Log.Severity, results_pb2.Log] = ...
  errors: List[results_pb2.Error] = ...
  files: List[results_pb2.File] = ...
  artifact_extensions: List[results_pb2.ArtifactExtension] = ...
  measurements: List[results_pb2.Measurement] = ...
  measurement_series: Dict[str, MeasurementSeriesOutput] = ...
  diagnoses: List[results_pb2.Diagnosis] = ...


class TestRunOutput:

  def __init__(self):
    ...

  def AddOutputArtifact(self, artifact: results_pb2.OutputArtifact):
    ...

  start: Optional[results_pb2.TestRunStart] = ...
  end: Optional[results_pb2.TestRunEnd] = ...
  logs: Dict[results_pb2.Log.Severity, List[results_pb2.Log]] = ...
  errors: List[results_pb2.Error] = ...
  tags: List[results_pb2.Tag] = ...
  steps: Dict[str, TestStepOutput] = ...
