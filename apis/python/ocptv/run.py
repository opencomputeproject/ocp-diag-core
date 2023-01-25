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
    Error,
    TestStatus,
    TestResult,
    LogSeverity,
)
from .step import TestStep
from .dut import Dut, SoftwareInfo
from .output import ArtifactEmitter
from .api import export_api


@export_api
class TestRunError(RuntimeError):
    """
    The `TestRunError` can be raised with a context scope started by `TestRun.scope()`.
    Any instance will be caught and handled by the context and will result in ending the
    whole run with an error outcome.
    """

    __slots__ = ("status", "result")

    # this may be further restricted re. params
    def __init__(self, *, status: TestStatus, result: TestResult):
        self.status = status
        self.result = result


@export_api
class TestRun:
    """
    The `TestRun` object should be used as a stateful interface for the diagnosis run.
    It presents various methods to interact with the run itself or to make child artifacts.

    Usage:
    >>> run = ocptv.TestRun(name="test", version="1.0")
    >>> with run.scope(dut=ocptv.Dut(id="dut0")):
    >>>     pass

    For other usages, see the the `examples` folder in the root of the project.

    See spec: https://github.com/opencomputeproject/ocp-diag-core/tree/main/json_spec#test-run-artifacts
    """

    def __init__(
        self,
        name: str,
        version: str,
        *,
        command_line: ty.Optional[str] = None,
        parameters: ty.Optional[ty.Dict[str, ty.Union[str, int]]] = None,
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

        # TODO: threadsafe?
        self._step_id: int = 0

    # by this point, user code has identified complete dut info or error'd out
    def start(self, *, dut: Dut):
        """
        Signal the test run start and emit a json `testRunStart` artifact.

        The device-under-test information is considered known at this point, and can
        be specified using the `dut_info` parameter.
        """
        start = RunStart(
            name=self.name,
            version=self._version,
            command_line=self.command_line,
            parameters=self._params,
            dut_info=dut.to_spec(),
        )
        self._emitter.emit(RunArtifact(impl=start))

    def end(self, *, status: TestStatus, result: TestResult):
        end = RunEnd(
            status=status,
            result=result,
        )
        self._emitter.emit(RunArtifact(impl=end))

    @contextmanager
    def scope(self, *, dut: Dut):
        try:
            yield self.start(dut=dut)
        except TestRunError as re:
            self.end(status=re.status, result=re.result)
        else:
            self.end(status=TestStatus.COMPLETE, result=TestResult.PASS)

    def add_step(self, name: str) -> TestStep:
        step = TestStep(name, step_id=self._step_id, emitter=self._emitter)
        self._step_id += 1
        return step

    def add_log(self, severity: LogSeverity, message: str):
        log = Log(
            severity=severity,
            message=message,
        )
        self._emitter.emit(RunArtifact(impl=log))

    def add_error(
        self,
        *,
        symptom: str,
        message: ty.Optional[str] = None,
        software_infos: ty.Optional[ty.List[SoftwareInfo]] = None,
    ):
        if software_infos is None:
            software_infos = []

        error = Error(
            symptom=symptom,
            message=message,
            software_infos=[o.to_spec() for o in software_infos],
        )
        self._emitter.emit(RunArtifact(impl=error))

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
    def parameters(self) -> ty.Optional[ty.Dict[str, ty.Union[str, int]]]:
        return self._params
