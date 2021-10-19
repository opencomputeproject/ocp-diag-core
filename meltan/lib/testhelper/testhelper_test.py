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

"""Unit tests for meltan.servers.usbdisconnect.testhelper.testhelper."""

from unittest import mock

from google.protobuf import json_format
from google.protobuf import message
import unittest
from meltan.lib.results import results_pb2
from meltan.lib.results.python import results
from meltan.lib.testhelper import test
from meltan.lib.testhelper import testhelper
from meltan.lib.testhelper import teststep
from third_party.pybind11_abseil import status as error

_STATUS_CODE_UNIMPLEMENTED = 12


class TestHelperTest(unittest.TestCase):

  def setUp(self):
    super(TestHelperTest, self).setUp()
    self._test_autospec = mock.create_autospec(test.Test)
    self._test = self._test_autospec.return_value
    self._test.GetDutName.return_value = 'DUT'
    self._params_autospec = mock.create_autospec(message.Message)
    self._params = self._params_autospec.return_value
    self._test_step_autospec = mock.create_autospec(teststep.TestStep)
    self._test_step = self._test_step_autospec.return_value

  @mock.patch(
      'meltan.lib.results.python.results.InitTestRun',
      autospec=results.TestRun)
  @mock.patch('builtins.print')
  def testRunnerInitFailed(self, mock_print, mock_init_run):
    mock_init_run.side_effect = error.StatusNotOk(_STATUS_CODE_UNIMPLEMENTED,
                                                  'Not supported')
    testhelper.TestHelper.StartAndRunTest(self._test, self._params)
    mock_print.assert_called_once()

  @mock.patch(
      'meltan.lib.results.python.results.InitTestRun',
      autospec=results.TestRun)
  @mock.patch(
      'meltan.lib.params.utils.GetParams',
      autospec=message.Message)
  def testGetParamsFailure(self, mock_get_params, mock_init_run):
    mock_get_params.side_effect = json_format.ParseError('Not supported')
    runner = mock_init_run.return_value
    testhelper.TestHelper.StartAndRunTest(self._test, self._params)
    runner.AddError.assert_called_once_with(
        'procedural-error', 'Failed to parse parameters: Not supported')
    runner.End.assert_called_once()

  @mock.patch(
      'meltan.lib.results.python.results.BeginTestStep',
      autospec=results.TestStep)
  @mock.patch(
      'meltan.lib.results.python.results.InitTestRun',
      autospec=results.TestRun)
  @mock.patch(
      'meltan.lib.params.utils.GetParams',
      autospec=message.Message)
  def testStepInitFailed(self, mock_get_params, mock_init_run, mock_step):
    mock_step.side_effect = error.StatusNotOk(_STATUS_CODE_UNIMPLEMENTED,
                                              'Not supported')
    params = mock_get_params.return_value
    runner = mock_init_run.return_value
    hwinfo_map = dict()
    self._test.GetHwInfoMap.return_value = hwinfo_map
    swinfo_map = dict()
    self._test.GetSwInfoMap.return_value = swinfo_map
    testhelper.TestHelper.StartAndRunTest(self._test, self._params)
    self._test.Setup.assert_called_once_with(runner, params)
    runner.StartAndRegisterInfos.assert_called_once_with([mock.ANY], params)
    runner.AddError.assert_called_once_with('procedural-error', mock.ANY)
    runner.End.assert_called_once()

  @mock.patch(
      'meltan.lib.results.python.results.BeginTestStep',
      autospec=results.TestStep)
  @mock.patch(
      'meltan.lib.results.python.results.InitTestRun',
      autospec=results.TestRun)
  @mock.patch(
      'meltan.lib.params.utils.GetParams',
      autospec=message.Message)
  def testSetupErrorException(self, mock_get_params, mock_init_run,
                              mock_step):
    mock_step.side_effect = error.StatusNotOk(_STATUS_CODE_UNIMPLEMENTED,
                                              'Not supported')
    params = mock_get_params.return_value
    runner = mock_init_run.return_value
    self._test.Setup.side_effect = test.FailedError('AnyError')
    testhelper.TestHelper.StartAndRunTest(self._test, self._params)
    self._test.Setup.assert_called_once_with(runner, params)
    self._test.GetDutName.assert_not_called()
    self._test.GetHwInfoMap.assert_not_called()
    self._test.GetSwInfoMap.assert_not_called()
    runner.StartAndRegisterInfos.assert_not_called()
    runner.End.assert_called_once()

  @mock.patch(
      'meltan.lib.results.python.results.BeginTestStep',
      autospec=results.TestStep)
  @mock.patch(
      'meltan.lib.results.python.results.InitTestRun',
      autospec=results.TestRun)
  @mock.patch(
      'meltan.lib.params.utils.GetParams',
      autospec=message.Message)
  def testSetupSkipException(self, mock_get_params, mock_init_run,
                             mock_step):
    mock_step.side_effect = error.StatusNotOk(_STATUS_CODE_UNIMPLEMENTED,
                                              'Not supported')
    params = mock_get_params.return_value
    runner = mock_init_run.return_value
    self._test.Setup.side_effect = test.SkipError('AnyError')
    testhelper.TestHelper.StartAndRunTest(self._test, self._params)
    self._test.Setup.assert_called_once_with(runner, params)
    self._test.GetDutName.assert_not_called()
    self._test.GetHwInfoMap.assert_not_called()
    self._test.GetSwInfoMap.assert_not_called()
    runner.StartAndRegisterInfos.assert_not_called()
    runner.Skip.assert_called_once()

  @mock.patch(
      'meltan.lib.results.python.results.BeginTestStep',
      autospec=results.TestStep)
  @mock.patch(
      'meltan.lib.results.python.results.InitTestRun',
      autospec=results.TestRun)
  @mock.patch(
      'meltan.lib.params.utils.GetParams',
      autospec=message.Message)
  def testWithSteps(self, mock_get_params, mock_init_run, mock_step):
    params = mock_get_params.return_value
    runner = mock_init_run.return_value
    step = mock_step.return_value
    hwinfo_map = dict()
    hw1info = results_pb2.HardwareInfo()
    hw1info.name = 'hw1'
    hwinfo_map['hw1'] = hw1info
    hw2info = results_pb2.HardwareInfo()
    hw2info.name = 'hw2'
    hwinfo_map['hw2'] = hw2info
    self._test.GetHwInfoMap.return_value = hwinfo_map
    swinfo_map = dict()
    sw1info = results_pb2.SoftwareInfo()
    sw1info.name = 'sw1'
    swinfo_map['sw1'] = sw1info
    sw2info = results_pb2.SoftwareInfo()
    sw2info.name = 'sw2'
    swinfo_map['sw2'] = sw2info
    self._test.GetSwInfoMap.return_value = swinfo_map
    self._test.NextStep.side_effect = [self._test_step, StopIteration]
    testhelper.TestHelper.StartAndRunTest(self._test, self._params)
    self._test.Setup.assert_called_once_with(runner, params)
    runner.StartAndRegisterInfos.assert_called_once_with([mock.ANY], params)
    self.assertEqual(self._test.NextStep.call_count, 2)
    mock_step.assert_called_once()
    self._test_step.Setup.assert_called_once()
    self._test_step.Run.assert_called_once()
    step.End.assert_called_once()
    runner.End.assert_called_once()

  @mock.patch(
      'meltan.lib.results.python.results.BeginTestStep',
      autospec=results.TestStep)
  @mock.patch(
      'meltan.lib.results.python.results.InitTestRun',
      autospec=results.TestRun)
  @mock.patch(
      'meltan.lib.params.utils.GetParams',
      autospec=message.Message)
  def testStepSetupError(self, mock_get_params, mock_init_run, mock_step):
    params = mock_get_params.return_value
    runner = mock_init_run.return_value
    step = mock_step.return_value
    hwinfo_map = dict()
    hw1info = results_pb2.HardwareInfo()
    hw1info.name = 'hw1'
    hwinfo_map['hw1'] = hw1info
    hw2info = results_pb2.HardwareInfo()
    hw2info.name = 'hw2'
    hwinfo_map['hw2'] = hw2info
    self._test.GetHwInfoMap.return_value = hwinfo_map
    swinfo_map = dict()
    sw1info = results_pb2.SoftwareInfo()
    sw1info.name = 'sw1'
    swinfo_map['sw1'] = sw1info
    sw2info = results_pb2.SoftwareInfo()
    sw2info.name = 'sw2'
    swinfo_map['sw2'] = sw2info
    self._test.GetSwInfoMap.return_value = swinfo_map
    self._test.NextStep.side_effect = [self._test_step, StopIteration]
    self._test_step.Setup.side_effect = teststep.FailedError('AnyError')
    testhelper.TestHelper.StartAndRunTest(self._test, self._params)
    self._test.Setup.assert_called_once_with(runner, params)
    runner.StartAndRegisterInfos.assert_called_once_with([mock.ANY], params)
    self.assertEqual(self._test.NextStep.call_count, 2)
    mock_step.assert_called_once()
    self._test_step.Setup.assert_called_once()
    self._test_step.Run.assert_not_called()
    step.End.assert_called_once()
    runner.End.assert_called_once()

  @mock.patch(
      'meltan.lib.results.python.results.BeginTestStep',
      autospec=results.TestStep)
  @mock.patch(
      'meltan.lib.results.python.results.InitTestRun',
      autospec=results.TestRun)
  @mock.patch(
      'meltan.lib.params.utils.GetParams',
      autospec=message.Message)
  def testStepSetupSkip(self, mock_get_params, mock_init_run, mock_step):
    params = mock_get_params.return_value
    runner = mock_init_run.return_value
    step = mock_step.return_value
    hwinfo_map = dict()
    hw1info = results_pb2.HardwareInfo()
    hw1info.name = 'hw1'
    hwinfo_map['hw1'] = hw1info
    hw2info = results_pb2.HardwareInfo()
    hw2info.name = 'hw2'
    hwinfo_map['hw2'] = hw2info
    self._test.GetHwInfoMap.return_value = hwinfo_map
    swinfo_map = dict()
    sw1info = results_pb2.SoftwareInfo()
    sw1info.name = 'sw1'
    swinfo_map['sw1'] = sw1info
    sw2info = results_pb2.SoftwareInfo()
    sw2info.name = 'sw2'
    swinfo_map['sw2'] = sw2info
    self._test.GetSwInfoMap.return_value = swinfo_map
    self._test.NextStep.side_effect = [self._test_step, StopIteration]
    self._test_step.Setup.side_effect = teststep.SkipError('AnyError')
    testhelper.TestHelper.StartAndRunTest(self._test, self._params)
    self._test.Setup.assert_called_once_with(runner, params)
    runner.StartAndRegisterInfos.assert_called_once_with([mock.ANY], params)
    self.assertEqual(self._test.NextStep.call_count, 2)
    mock_step.assert_called_once()
    self._test_step.Setup.assert_called_once()
    self._test_step.Run.assert_not_called()
    step.Skip.assert_called_once()
    runner.End.assert_called_once()

  @mock.patch(
      'meltan.lib.results.python.results.BeginTestStep',
      autospec=results.TestStep)
  @mock.patch(
      'meltan.lib.results.python.results.DutInfo',
      autospec=results.DutInfo)
  @mock.patch(
      'meltan.lib.results.python.results.InitTestRun',
      autospec=results.TestRun)
  @mock.patch(
      'meltan.lib.params.utils.GetParams',
      autospec=message.Message)
  def testStepRunSkip(self, mock_get_params, mock_init_run, mock_dut_info,
                      mock_step):
    dut_info = mock_dut_info.return_value
    params = mock_get_params.return_value
    runner = mock_init_run.return_value
    step = mock_step.return_value
    hwinfo_map = dict()
    hw1info = results_pb2.HardwareInfo()
    hw1info.name = 'hw1'
    hwinfo_map['hw1'] = hw1info
    hw2info = results_pb2.HardwareInfo()
    hw2info.name = 'hw2'
    hwinfo_map['hw2'] = hw2info
    self._test.GetHwInfoMap.return_value = hwinfo_map
    swinfo_map = dict()
    sw1info = results_pb2.SoftwareInfo()
    sw1info.name = 'sw1'
    swinfo_map['sw1'] = sw1info
    sw2info = results_pb2.SoftwareInfo()
    sw2info.name = 'sw2'
    swinfo_map['sw2'] = sw2info
    self._test.GetSwInfoMap.return_value = swinfo_map
    self._test.NextStep.side_effect = [self._test_step, StopIteration]
    self._test_step.Run.side_effect = teststep.SkipError('AnyError')
    testhelper.TestHelper.StartAndRunTest(self._test, self._params)
    self._test.Setup.assert_called_once_with(runner, params)
    self.assertEqual(dut_info.AddHardware.call_count, 2)
    call_list = [mock.call(hw1info), mock.call(hw2info)]
    dut_info.AddHardware.assert_has_calls(call_list)
    self.assertEqual(dut_info.AddSoftware.call_count, 2)
    call_list = [mock.call(sw1info), mock.call(sw2info)]
    dut_info.AddSoftware.assert_has_calls(call_list)
    runner.StartAndRegisterInfos.assert_called_once_with([dut_info], params)
    self.assertEqual(self._test.NextStep.call_count, 2)
    mock_step.assert_called_once()
    self._test_step.Setup.assert_called_once()
    step.Skip.assert_called_once()
    runner.End.assert_called_once()

  @mock.patch(
      'meltan.lib.results.python.results.BeginTestStep',
      autospec=results.TestStep)
  @mock.patch(
      'meltan.lib.results.python.results.DutInfo',
      autospec=results.DutInfo)
  @mock.patch(
      'meltan.lib.results.python.results.InitTestRun',
      autospec=results.TestRun)
  @mock.patch(
      'meltan.lib.params.utils.GetParams',
      autospec=message.Message)
  def testStepRunError(self, mock_get_params, mock_init_run, mock_dut_info,
                       mock_step):
    dut_info = mock_dut_info.return_value
    params = mock_get_params.return_value
    runner = mock_init_run.return_value
    step = mock_step.return_value
    hwinfo_map = dict()
    hw1info = results_pb2.HardwareInfo()
    hw1info.name = 'hw1'
    hwinfo_map['hw1'] = hw1info
    hw2info = results_pb2.HardwareInfo()
    hw2info.name = 'hw2'
    hwinfo_map['hw2'] = hw2info
    self._test.GetHwInfoMap.return_value = hwinfo_map
    swinfo_map = dict()
    sw1info = results_pb2.SoftwareInfo()
    sw1info.name = 'sw1'
    swinfo_map['sw1'] = sw1info
    sw2info = results_pb2.SoftwareInfo()
    sw2info.name = 'sw2'
    swinfo_map['sw2'] = sw2info
    self._test.GetSwInfoMap.return_value = swinfo_map
    self._test.NextStep.side_effect = [self._test_step, StopIteration]
    self._test_step.Run.side_effect = teststep.FailedError('AnyError')
    testhelper.TestHelper.StartAndRunTest(self._test, self._params)
    self._test.Setup.assert_called_once_with(runner, params)
    self.assertEqual(dut_info.AddHardware.call_count, 2)
    call_list = [mock.call(hw1info), mock.call(hw2info)]
    dut_info.AddHardware.assert_has_calls(call_list)
    self.assertEqual(dut_info.AddSoftware.call_count, 2)
    call_list = [mock.call(sw1info), mock.call(sw2info)]
    dut_info.AddSoftware.assert_has_calls(call_list)
    runner.StartAndRegisterInfos.assert_called_once_with([dut_info], params)
    self.assertEqual(self._test.NextStep.call_count, 2)
    mock_step.assert_called_once()
    self._test_step.Setup.assert_called_once()
    step.End.assert_called_once()
    runner.End.assert_called_once()

if __name__ == '__main__':
  unittest.main()
