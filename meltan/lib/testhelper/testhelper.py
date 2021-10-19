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

"""testhelper class, setup and run Meltan tests."""

import sys

from google.protobuf import json_format
from google.protobuf import message
from meltan.lib.params import utils
from meltan.lib.results.python import results
from meltan.lib.testhelper import test
from meltan.lib.testhelper import teststep
from third_party.pybind11_abseil import status as error


class TestHelper():
  """TestHelper Class."""

  @staticmethod
  def StartAndRunTest(test_run: test.Test, params_msg: message.Message) -> None:
    """Function to start and run test.

    This function starts, setup and run the test and test steps.

    Args:
      test_run: A test object derived from abstract test.Test object.
      params_msg: params_msg to denote the params format of the test.
    """
    # Test runner setup.
    try:
      runner = results.InitTestRun(type(test_run).__name__)
    except error.StatusNotOk as e:
      print('Init Test Run Failed %s ' % e, file=sys.stderr)
      return

    # Parse the input parameters.
    try:
      params = utils.GetParams(params_msg)
    except json_format.ParseError as e:
      runner.AddError('procedural-error',
                      'Failed to parse parameters: {0}'.format(e))
      runner.End()
      return

    try:
      test_run.Setup(runner, params)
    except test.FailedError:
      runner.End()
      return
    except test.SkipError:
      runner.Skip()
      return

    dut_name = test_run.GetDutName()
    hwinfomap = test_run.GetHwInfoMap()
    swinfomap = test_run.GetSwInfoMap()
    dut_info = results.DutInfo(dut_name)
    hwrecordmap = dict()
    swrecordmap = dict()

    for key in hwinfomap:
      hw_record = dut_info.AddHardware(hwinfomap[key])
      hwrecordmap[key] = hw_record

    for key in swinfomap:
      sw_record = dut_info.AddSoftware(swinfomap[key])
      swrecordmap[key] = sw_record

    runner.StartAndRegisterInfos([dut_info], params)

    while True:
      try:
        test_step = test_run.NextStep()
        # Test step.
        try:
          step_name = test_step.GetStepName()
          step = results.BeginTestStep(runner, step_name)
        except error.StatusNotOk as e:
          runner.AddError(
              'procedural-error', 'Failed to begin ' +
              step_name + ' step: {0}'.format(e))
          runner.End()
          return
        try:
          test_step.Setup(runner, step, hwrecordmap, swrecordmap)
          test_step.Run()
          step.End()
        except teststep.FailedError:
          step.End()
        except teststep.SkipError:
          step.Skip()
      except StopIteration:
        break

    runner.End()
