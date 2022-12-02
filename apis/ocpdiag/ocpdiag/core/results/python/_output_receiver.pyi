# Copyright 2022 Google LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

from typing import Callable, Optional, Any

from ocpdiag.core.results import results_pb2
from ocpdiag.core.results import results_model_pb2
from ocpdiag.core.results.python import results


class OutputArtifactIterator:

  def __next__(self) -> results_pb2.OutputArtifact:
    ...


class OutputArtifactContainer:

  def __iter__(self) -> OutputArtifactIterator:
    ...

  @property
  def file_path(self) -> str:
    ...


class OutputReceiver:

  def __init__(self):
    ...

  @property
  def test_run(self) -> results_model_pb2.TestRunModel:
    ...

  @property
  def raw(self) -> OutputArtifactContainer:
    ...

  @property
  def artifact_writer(self) -> results.ArtifactWriter:
    ...
