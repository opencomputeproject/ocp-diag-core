"""
This module describes the test steps inside the test run.
"""
from contextlib import contextmanager
import typing as ty

from .objects import (
    StepArtifact,
    StepStart,
    StepEnd,
    Log,
    Error,
    Diagnosis,
    TestStatus,
    LogSeverity,
    DiagnosisType,
)
from .output import ArtifactEmitter


class TestStepError(RuntimeError):
    __slots__ = "status"

    def __init__(self, *, status: TestStatus):
        self.status = status


class TestStep:
    """
    TODO:
    Should not be used directly by user code, but through TestRun.add_step().
    """

    def __init__(self, name: str, *, step_id: int, emitter: ArtifactEmitter):
        self._name = name
        self._id = step_id
        self._idstr = "{}".format(step_id)
        self._emitter = emitter

    def start(self):
        start = StepStart(name=self._name)
        self._emitter.emit(StepArtifact(id=self._idstr, impl=start))

    def end(self, *, status: TestStatus):
        end = StepEnd(status=status)
        self._emitter.emit(StepArtifact(id=self._idstr, impl=end))

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

    # TODO: fix signature
    def add_diagnosis(
        self,
        diagnosis_type: DiagnosisType,
        *,
        verdict: str,
        message: ty.Optional[str] = None,
        hardware_info_id: ty.Optional[str] = None,
        subcomponent=None,
    ):
        diag = Diagnosis(
            verdict=verdict,
            type=diagnosis_type,
            message=message,
        )
        self._emitter.emit(StepArtifact(id=self._idstr, impl=diag))

    # TODO: fix software_info_ids when duts are done
    def add_error(
        self,
        *,
        symptom: str,
        message: ty.Optional[str] = None,
        software_info_ids: ty.Optional[list[str]] = None,
    ):
        error = Error(
            symptom=symptom,
            message=message,
            software_info_ids=[],
        )
        self._emitter.emit(StepArtifact(id=self._idstr, impl=error))

    # def add_file():
    #     pass

    def add_log(self, severity: LogSeverity, message: str):
        log = Log(
            severity=severity,
            message=message,
        )
        self._emitter.emit(StepArtifact(id=self._idstr, impl=log))

    @property
    def name(self) -> str:
        return self._name

    @property
    def stepId(self) -> int:
        return self._id
