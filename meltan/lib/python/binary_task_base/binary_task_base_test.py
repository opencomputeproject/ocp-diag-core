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

"""Unit tests for binary_task_base."""

import io
import os
import subprocess
from unittest import mock

from absl.testing import absltest
import dataclasses

from google.protobuf import duration_pb2
from meltan.lib.python.binary_task_base import binary_task_base
from meltan.lib.results.python import results


@dataclasses.dataclass
class Params:
  filename_stdout: str
  filename_stderr: str
  runtime: duration_pb2.Duration
  overhead: duration_pb2.Duration


class BinaryTaskTest(absltest.TestCase):

  def setUp(self):
    super().setUp()

    # Simulate successful binaries by default.
    self._mock_popen = mock.patch.object(
        subprocess, "Popen", autospec=True).start()
    self._mock_popen.return_value.wait.return_value = 0
    self._mock_popen.return_value.stdout = io.StringIO("")
    self._mock_popen.return_value.stderr = io.StringIO("")

    self._workdir = self.create_tempdir()
    self._stdout_filepath = os.path.join(self._workdir, "stdout.txt")
    self._stderr_filepath = os.path.join(self._workdir, "stderr.txt")

    self._params = Params(
        filename_stdout=self._stdout_filepath,
        filename_stderr=self._stderr_filepath,
        runtime=duration_pb2.Duration(seconds=300),
        overhead=duration_pb2.Duration(seconds=200))

    self._mock_step = mock.create_autospec(results.BeginTestStep)
    self._task = binary_task_base.BinaryTask(
        self._mock_step, "dummy_name", "dummy_binary", use_text_mode=True)
    self._mock_step.AddError = mock.Mock()
    self._mock_step.LogError = mock.Mock()
    self._mock_step.LogInfo = mock.Mock()
    self._mock_step.LogWarn = mock.Mock()

    self.addCleanup(mock.patch.stopall)

  def testExec(self):
    stdout_value = "info log"
    stderr_value = "error log"
    self._mock_popen.return_value.stdout = io.StringIO(stdout_value)
    self._mock_popen.return_value.stderr = io.StringIO(stderr_value)

    self._task.Exec(self._params)
    self._mock_popen.assert_called_once_with([self._task.binary],
                                             stdout=subprocess.PIPE,
                                             stderr=subprocess.PIPE,
                                             bufsize=1,
                                             encoding="utf-8")

    with open(self._stdout_filepath, "r") as stdout:
      self.assertEqual(stdout.read(), stdout_value)
    with open(self._stderr_filepath, "r") as stderr:
      self.assertEqual(stderr.read(), stderr_value)

  def testExecFail_PreExecutionError(self):
    message = "dummy message"
    symptom = "dummy symptom"
    with mock.patch.object(
        binary_task_base.BinaryTask,
        "PreExecution",
        autospec=True,
        side_effect=binary_task_base.PreExecutionError(message, symptom)):
      self._task.Exec(self._params)
      self._mock_step.AddError.assert_has_calls(
          [mock.call(symptom, message, [])])

  def testExecFail_GenerateCmdError(self):
    message = "dummy message"
    symptom = "dummy symptom"
    with mock.patch.object(
        binary_task_base.BinaryTask,
        "GenerateCmd",
        autospec=True,
        side_effect=binary_task_base.GenerateCmdError(message, symptom)):
      self._task.Exec(self._params)
      self._mock_step.AddError.assert_has_calls(
          [mock.call(symptom, message, [])])

  def testExecFail_ReturnCode(self):
    msg = "Command: {}, Return Code: {}".format("dummy_binary", -2)
    self._mock_popen.return_value.wait.return_value = -2
    self._task.Exec(self._params)
    self._mock_step.LogError.assert_has_calls([mock.call(msg)])

  def testExecFail_SubprocessTimeout(self):
    self._mock_popen.return_value.wait.side_effect = subprocess.TimeoutExpired(
        "Times out", timeout=100)
    self._task.Exec(self._params)
    self._mock_step.AddError.assert_has_calls([
        mock.call(
            f"{self._task.taskname}-timeout",
            mock.ANY, [])
    ])


if __name__ == "__main__":
  absltest.main()
