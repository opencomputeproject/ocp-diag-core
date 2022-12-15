"""
This module describes the test steps inside the test run.
"""
from contextlib import contextmanager

from .objects import TempArtifactModel, TestStatus, LogSeverity, DiagnosisType
from .output import ArtifactEmitter


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
