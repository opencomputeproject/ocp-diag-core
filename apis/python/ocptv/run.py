"""
This module describes the high level test run and related objects.
"""
import sys
from contextlib import contextmanager
import typing as ty

from .objects import (
    RunArtifact,
    RunStart,
    RunEnd,
    Log,
    TestStatus,
    TestResult,
    LogSeverity,
)
from .step import TestStep
from .dut import Dut
from .output import ArtifactEmitter


class TestRunError(RuntimeError):
    __slots__ = ("status", "result")

    # this may be further restricted re. params
    def __init__(self, *, status: TestStatus, result: TestResult):
        self.status = status
        self.result = result


class TestRun:
    def __init__(
        self,
        name: str,
        version: str,
        *,
        command_line: ty.Optional[str] = None,
        parameters: ty.Optional[dict[str, str | int]] = None,
    ):
        """
        Make a new stateful test run model object. Parameters given to init
        are assumed to be known at object creation time, anything else, like things
        that have a discovery process behind them, are specified in the start() call.
        """
        self._name = name
        self._version = version

        if command_line is not None:
            self._cmdline = command_line
        else:
            self._cmdline = self._get_cmdline()

        self._params = parameters or {}
        self._emitter = ArtifactEmitter()

        self._stepId = 0

    # by this point, user code has identified complete dut info or error'd out
    def start(self, *, duts: ty.Optional[list[Dut]] = None):
        """
        Signal the test run start and emit a json `testRunStart` artifact.

        The device-under-test information is considered known at this point, and can
        be specified using the `dut_info` parameter.
        """
        if duts is None:
            duts = []

        start = RunStart(
            name=self.name,
            version=self._version,
            command_line=self.command_line,
            parameters=self._params,
            dut_info=[x.info for x in duts],
        )
        self._emitter.emit(RunArtifact(impl=start))

    def end(self, *, status: TestStatus, result: TestResult):
        end = RunEnd(
            status=status,
            result=result,
        )
        self._emitter.emit(RunArtifact(impl=end))

    @contextmanager
    def scope(self, *, duts: ty.Optional[list[Dut]] = None):
        try:
            yield self.start(duts=duts)
        except TestRunError as re:
            self.end(status=re.status, result=re.result)
        else:
            self.end(status=TestStatus.COMPLETE, result=TestResult.PASS)

    def add_step(self, name: str) -> TestStep:
        step = TestStep(name, step_id=self._stepId, emitter=self._emitter)
        self._stepId += 1
        return step

    def add_log(self, severity: LogSeverity, message: str):
        log = Log(
            severity=severity,
            message=message,
        )
        self._emitter.emit(RunArtifact(impl=log))

    # def add_error(self, symptom: str, message: str, software_info_ids):
    #     pass

    def _get_cmdline(self):
        return " ".join(sys.argv[1:])

    # API COMMENT: make stuff immutable once constructed
    @property
    def name(self) -> str:
        return self._name

    @property
    def version(self) -> str:
        return self._version

    @property
    def command_line(self) -> str:
        return self._cmdline

    @property
    def parameters(self) -> ty.Optional[dict[str, str | int]]:
        return self._params
