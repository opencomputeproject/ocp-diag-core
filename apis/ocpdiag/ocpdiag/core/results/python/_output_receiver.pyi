# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

from typing import Callable, Optional

from ocpdiag.core.results import results_pb2
from ocpdiag.core.results.python import output_model
from ocpdiag.core.results.python import results


class OutputArtifactIterator:

  def __next__(self) -> results_pb2.OutputArtifact:
    ...


class OutputReceiver:

  def __init__(self):
    ...

  def __iter__(self) -> OutputArtifactIterator:
    ...

  @property
  def artifact_writer(self) -> results.ArtifactWriter:
    ...

  @property
  def model(self) -> output_model.TestRunOutput:
    ...