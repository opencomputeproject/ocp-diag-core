import time
from contextlib import contextmanager
import typing as ty

from .objects import (
    StepArtifact,
    MeasurementValueType,
    MeasurementSeriesStart,
    MeasurementSeriesEnd,
    MeasurementSeriesElement,
    MeasurementSeriesType,
    Validator as ValidatorSpec,
    ValidatorType,
    ValidatorValueType,
    Metadata,
)
from .dut import Subcomponent, HardwareInfo
from .output import ArtifactEmitter
from .api import export_api


class MeasurementSeriesEmitter(ArtifactEmitter):
    def __init__(self, step_id: str, emitter: ArtifactEmitter):
        self._step_id = step_id
        self._emitter = emitter

    def emit_impl(self, impl: MeasurementSeriesType):
        self._emitter.emit(StepArtifact(id=self._step_id, impl=impl))


# Following object is a proxy type so we get future flexibility, avoiding the usage
# of the low-level models.
@export_api
class Validator:
    """
    The `Validator` object represent a named validation that is relevant to a measurement or
    measurement series.

    ref: https://github.com/opencomputeproject/ocp-diag-core/tree/main/json_spec#validator
    """

    def __init__(
        self,
        *,
        type: ValidatorType,
        value: ValidatorValueType,
        name: ty.Optional[str] = None,
        metadata: ty.Optional[Metadata] = None,
    ):
        self._spec_object = ValidatorSpec(
            name=name,
            type=type,
            value=value,
            metadata=metadata,
        )

    def to_spec(self) -> ValidatorSpec:
        return self._spec_object


class MeasurementSeries:
    """
    The `MeasurementSeries` instances model a specific time-based list of values relevant to the diagnostic.
    A series is started by default on instantiation and must be ended manually with the `.end()` method or
    by using a `.scope()` context manager.

    Instances of this type must only be created by calls to `TestStep.start_measurement_series()`.

    ref: https://github.com/opencomputeproject/ocp-diag-core/tree/main/json_spec#measurementseriesstart
    """

    def __init__(
        self,
        emitter: MeasurementSeriesEmitter,
        series_id: str,
        *,
        name: str,
        unit: ty.Optional[str] = None,
        validators: ty.Optional[ty.List[Validator]] = None,
        hardware_info: ty.Optional[HardwareInfo] = None,
        subcomponent: ty.Optional[Subcomponent] = None,
        metadata: ty.Optional[Metadata] = None,
    ):
        self._emitter = emitter
        self._id = series_id

        # TODO: threadsafety?
        self._index: int = 0

        self._start(name, unit, validators, hardware_info, subcomponent, metadata)

    def add_measurement(
        self,
        *,
        value: MeasurementValueType,
        timestamp: ty.Optional[float] = None,
        metadata: ty.Optional[Metadata] = None,
    ):
        if timestamp is None:
            # use local time if not specified
            timestamp = time.time()

        measurement = MeasurementSeriesElement(
            index=self._index,
            value=value,
            timestamp=timestamp,
            series_id=self._id,
            metadata=metadata,
        )
        self._emitter.emit_impl(measurement)
        self._index += 1

    def _start(
        self,
        name: str,
        unit: ty.Optional[str] = None,
        validators: ty.Optional[ty.List[Validator]] = None,
        hardware_info: ty.Optional[HardwareInfo] = None,
        subcomponent: ty.Optional[Subcomponent] = None,
        metadata: ty.Optional[Metadata] = None,
    ):
        if validators is None:
            validators = []

        start = MeasurementSeriesStart(
            name=name,
            unit=unit,
            series_id=self._id,
            validators=[v.to_spec() for v in validators],
            hardware_info=hardware_info.to_spec() if hardware_info else None,
            subcomponent=subcomponent.to_spec() if subcomponent else None,
            metadata=metadata,
        )
        self._emitter.emit_impl(start)

    def end(self):
        end = MeasurementSeriesEnd(
            series_id=self._id,
            total_count=self._index,
        )
        self._emitter.emit_impl(end)

    @contextmanager
    def scope(self):
        try:
            yield
        finally:
            self.end()
