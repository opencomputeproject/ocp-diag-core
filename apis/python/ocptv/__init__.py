import sys
import threading
import time
from abc import ABC, abstractmethod
from contextlib import contextmanager
from enum import Enum

from typing import Dict, Union


class OCPVersion(Enum):
    VERSION_2_0 = (2, 0)


class TestStatus(Enum):
    COMPLETE = 1
    ERROR = 2
    SKIP = 3


class TestResult(Enum):
    PASS = 1
    FAIL = 2
    NOT_APPLICABLE = 3


class LogSeverity(Enum):
    INFO = 1
    DEBUG = 2
    WARNING = 3
    ERROR = 4
    FATAL = 5


class DiagnosisType(Enum):
    PASS = 1
    FAIL = 2
    UNKNOWN = 3


class Writer(ABC):
    @abstractmethod
    def write(self, buffer: str):
        pass


class StdoutWriter(Writer):
    def write(self, buffer: str):
        print(buffer)


# module scoped output channel (similar to python logging)
_writer = StdoutWriter()


def configOutput(writer: Writer):
    global _writer
    _writer = writer


class TempArtifactModel:
    def __init__(self, n, v):
        self.name = n
        self.value = v


class ArtifactEmitter:
    def __init__(self):
        self._seq = 0
        self._lock = threading.Lock()

    # artifact here is probably a dataclass that can be serialized
    def emit(self, artifact: TempArtifactModel):
        if self._seq == 0:
            self._emit_version()

        # this sync point needs to happen before the write
        with self._lock:
            # multiprocess will have an issue here
            self._seq += 1

        _writer.write(
            f"""{{"{artifact.name}": {artifact.value}, "sequenceNumber": {self._seq}, "timestamp": {time.time()}}}"""
        )

    def _emit_version(self):
        major, minor = OCPVersion.VERSION_2_0.value
        version = f"""{{"major": {major}, "minor": {minor}}}"""
        _writer.write(
            f"""{{"schemaVersion": {version}, "sequenceNumber": 0, "timestamp": {time.time()}}}"""
        )


class TestStepError(RuntimeError):
    __slots__ = "status"

    def __init__(self, *, status: TestStatus):
        self.status = status


class TestStep:
    def __init__(self, name: str, *, stepId: int, emitter: ArtifactEmitter):
        self._name = name
        self._id = stepId
        self._emitter = emitter

    def start(self):
        json = f"""{{"testStepStart": {{"name": "{self._name}"}}, "testStepId": "{self._id}"}}"""
        self._emitter.emit(TempArtifactModel("testStepArtifact", json))

    def end(self, *, status: TestStatus):
        json = f"""{{"testStepEnd": {{"status": "{status.name}"}}, "testStepId": "{self._id}"}}"""
        self._emitter.emit(TempArtifactModel("testStepArtifact", json))

    @contextmanager
    def scope(self):
        try:
            yield self.start()
        except TestStepError as e:
            self.end(status=e.status)
        else:
            self.end(status=TestStatus.COMPLETE)

    # def add_measurement(self):
    #     pass

    # def add_measurement_series(self) -> MeasurementSeries:
    #     return MeasurementSeries()

    def add_diagnosis(
        self,
        diagType: DiagnosisType,
        *,
        verdict: str,
        message: str = None,
        hardwareInfoId: str = None,
        subcomponent=None,
    ):
        json = f"""{{"diagnosis": {{"type": "{diagType.name}", "verdict": "{verdict}"}}, "testStepId": "{self._id}"}}"""
        self._emitter.emit(TempArtifactModel("testStepArtifact", json))

    # def add_error(self, symptom: str, message: str, software_info_ids):
    #     pass

    # def add_file():
    #     pass

    def add_log(self, severity: LogSeverity, message: str):
        json = f"""{{"log": {{"severity": "{severity.name}", "message": "{message}"}}, "testStepId": "{self._id}"}}"""
        self._emitter.emit(TempArtifactModel("testStepArtifact", json))

    @property
    def name(self) -> str:
        return self._name

    @property
    def stepId(self) -> int:
        return self._id


class TestRunError(RuntimeError):
    __slots__ = ("status", "result")

    # this may be further restricted re. params
    def __init__(self, *, status: TestStatus, result: TestResult):
        self.status = status
        self.result = result


class DutInfo:
    def __str__(self):
        return "dut_info_here"


class TestRun:
    def __init__(
        self,
        name: str,
        version: str,
        *,
        command_line: str = None,
        parameters: Dict[str, Union[str, int]] = None,
    ):
        self._name = name
        self._version = version

        if command_line is not None:
            self._cmdline = command_line
        else:
            self._cmdline = self._get_cmdline()

        self._params = parameters
        self._emitter = ArtifactEmitter()

        self._stepId = 0

    # by this point, user code has identified complete dut info or error'd out
    def start(self, *, dut_info: DutInfo):
        json = f"""{{"testRunStart": {{"name": "{self.name}", "version": "{self.version}", "command_line": "{self.command_line}", "dut_info": "{dut_info}"}}}}"""
        self._emitter.emit(TempArtifactModel("testRunArtifact", json))

    def end(self, *, status: TestStatus, result: TestResult):
        json = f"""{{"testRunEnd": {{"status": "{status.name}", "result": "{result.name}"}}}}"""
        self._emitter.emit(TempArtifactModel("testRunArtifact", json))

    @contextmanager
    def scope(self, *, dut_info: DutInfo):
        try:
            yield self.start(dut_info=dut_info)
        except TestRunError as re:
            self.end(status=re.status, result=re.result)
        else:
            self.end(status=TestStatus.COMPLETE, result=TestResult.PASS)

    def add_step(self, name: str) -> TestStep:
        step = TestStep(name, stepId=self._stepId, emitter=self._emitter)
        self._stepId += 1
        return step

    def add_log(self, severity: LogSeverity, message: str):
        json = (
            f"""{{"log": {{"severity": "{severity.name}", "message": "{message}"}}}}"""
        )
        self._emitter.emit(TempArtifactModel("testRunArtifact", json))

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
    def parameters(self) -> Dict[str, Union[str, int]]:
        return self._params
