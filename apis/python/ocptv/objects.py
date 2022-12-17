"""
Low level object models that are to be serialized as JSON.
NOT PUBLIC API, these are not intended to be used by client code
unless explicitly exported as public in __init__.py

Developer notes:
A field can either have metadata.spec_field set or field.SPEC_OBJECT set, not both.
If SPEC_OBJECT is set, this field is an union type and serialization should take the
value in `SPEC_OBJECT` as the serialized field name. Otherwise, the metadata.spec_field
says what the serializer should use for field name.
In general, metadata.spec_field should only be present for primitive types.
"""
from enum import Enum
import dataclasses as dc
import typing as ty

from .formatter import format_enum, format_timestamp

# TODO: waiting on spec md PR to add all docstrings for objects and fields.


class ArtifactType(ty.Protocol):
    """
    Protocol type to describe all low level serializable objects in this file.
    """

    __dataclass_fields__: ty.ClassVar[dict]


class OCPVersion(Enum):
    VERSION_2_0 = (2, 0)


@dc.dataclass
class SchemaVersion:
    SPEC_OBJECT: ty.ClassVar[str] = "schemaVersion"

    major: int = dc.field(
        default=OCPVersion.VERSION_2_0.value[0],
        metadata={"spec_field": "major"},
    )

    minor: int = dc.field(
        default=OCPVersion.VERSION_2_0.value[1],
        metadata={"spec_field": "minor"},
    )


# NOTE: This is intentionally not a dataclass
class Metadata(dict):
    """
    Metadata container, is a type dict in order to accomodate
    any arbitrary data put into it.
    """

    SPEC_OBJECT: ty.ClassVar[str] = "metadata"


class LogSeverity(Enum):
    INFO = "INFO"
    DEBUG = "DEBUG"
    WARNING = "WARNING"
    ERROR = "ERROR"
    FATAL = "FATAL"


@dc.dataclass
class Log:
    SPEC_OBJECT: ty.ClassVar[str] = "log"

    severity: LogSeverity = dc.field(
        metadata={
            "spec_field": "severity",
            "formatter": format_enum,
        },
    )

    message: str = dc.field(
        metadata={"spec_field": "message"},
    )


@dc.dataclass
class Error:
    SPEC_OBJECT: ty.ClassVar[str] = "error"

    symptom: str = dc.field(
        metadata={"spec_field": "symptom"},
    )

    message: ty.Optional[str] = dc.field(
        metadata={"spec_field": "message"},
    )

    software_info_ids: list[str] = dc.field(
        metadata={"spec_field": "softwareInfoIds"},
    )


@dc.dataclass
class File:
    SPEC_OBJECT: ty.ClassVar[str] = "file"

    name: str = dc.field(
        metadata={"spec_field": "displayName"},
    )

    uri: str = dc.field(
        metadata={"spec_field": "uri"},
    )

    is_snapshot: bool = dc.field(
        metadata={"spec_field": "isSnapshot"},
    )

    description: ty.Optional[str] = dc.field(
        metadata={"spec_field": "description"},
    )

    content_type: ty.Optional[str] = dc.field(
        metadata={"spec_field": "contentType"},
    )

    metadata: ty.Optional[Metadata]


class DiagnosisType(Enum):
    PASS = "PASS"
    FAIL = "FAIL"
    UNKNOWN = "UNKNOWN"


@dc.dataclass
class Diagnosis:
    SPEC_OBJECT: ty.ClassVar[str] = "diagnosis"

    verdict: str = dc.field(
        metadata={"spec_field": "verdict"},
    )

    type: DiagnosisType = dc.field(
        metadata={
            "spec_field": "type",
            "formatter": format_enum,
        },
    )

    message: ty.Optional[str] = dc.field(
        metadata={"spec_field": "message"},
    )

    # TODO: hardwareInfoId
    # TODO: subcomponent


@dc.dataclass
class DutInfo:
    pass


MeasurementValueType = float | int | bool | str

ValidatorValueType = list["ValidatorValueType"] | MeasurementValueType


class ValidatorType(Enum):
    EQUAL = "EQUAL"
    NOT_EQUAL = "NOT_EQUAL"
    LESS_THAN = "LESS_THAN"
    LESS_THAN_OR_EQUAL = "LESS_THAN_OR_EQUAL"
    GREATER_THAN = "GREATER_THAN"
    GREATER_THAN_OR_EQUAL = "GREATER_THAN_OR_EQUAL"
    REGEX_MATCH = "REGEX_MATCH"
    REGEX_NO_MATCH = "REGEX_NO_MATCH"
    IN_SET = "IN_SET"
    NOT_IN_SET = "NOT_IN_SET"


@dc.dataclass
class Validator:
    SPEC_OBJECT: ty.ClassVar[str] = "validator"

    name: ty.Optional[str] = dc.field(
        metadata={"spec_field": "name"},
    )

    type: ValidatorType = dc.field(
        metadata={
            "spec_field": "type",
            "formatter": format_enum,
        },
    )

    value: ValidatorValueType = dc.field(
        metadata={"spec_field": "value"},
    )

    metadata: ty.Optional[Metadata]


@dc.dataclass
class Measurement:
    SPEC_OBJECT: ty.ClassVar[str] = "measurement"

    name: str = dc.field(
        metadata={"spec_field": "name"},
    )

    value: MeasurementValueType = dc.field(
        metadata={"spec_field": "value"},
    )

    unit: ty.Optional[str] = dc.field(
        metadata={"spec_field": "unit"},
    )

    validators: list[Validator] = dc.field(
        metadata={"spec_field": "validators"},
    )

    # TODO: make an obj here and formatter?
    # hardware_info_id: ty.Optional[str] = dc.field(
    #     metadata={"spec_field": "hardwareInfoId"},
    # )

    # subcomponent: ty.Optional[Subcomponent]
    metadata: ty.Optional[Metadata]


@dc.dataclass
class MeasurementSeriesStart:
    SPEC_OBJECT: ty.ClassVar[str] = "measurementSeriesStart"

    name: str = dc.field(
        metadata={"spec_field": "name"},
    )

    unit: ty.Optional[str] = dc.field(
        metadata={"spec_field": "unit"},
    )

    series_id: str = dc.field(
        metadata={"spec_field": "measurementSeriesId"},
    )

    validators: list[Validator] = dc.field(
        metadata={"spec_field": "validators"},
    )

    # TODO: make an obj here and formatter?
    # hardware_info_id: ty.Optional[str] = dc.field(
    #     metadata={"spec_field": "hardwareInfoId"},
    # )

    # subcomponent: ty.Optional[Subcomponent]
    metadata: ty.Optional[Metadata]


@dc.dataclass
class MeasurementSeriesEnd:
    SPEC_OBJECT: ty.ClassVar[str] = "measurementSeriesEnd"

    series_id: str = dc.field(
        metadata={"spec_field": "measurementSeriesId"},
    )

    total_count: int = dc.field(
        metadata={"spec_field": "totalCount"},
    )


@dc.dataclass
class MeasurementSeriesElement:
    SPEC_OBJECT: ty.ClassVar[str] = "measurementSeriesElement"

    index: int = dc.field(
        metadata={"spec_field": "index"},
    )

    value: MeasurementValueType = dc.field(
        metadata={"spec_field": "value"},
    )

    timestamp: float = dc.field(
        metadata={
            "spec_field": "timestamp",
            "formatter": format_timestamp,
        },
    )

    series_id: str = dc.field(
        metadata={"spec_field": "measurementSeriesId"},
    )

    metadata: ty.Optional[Metadata]


MeasurementSeriesType = (
    MeasurementSeriesStart | MeasurementSeriesEnd | MeasurementSeriesElement
)


@dc.dataclass
class RunStart:
    SPEC_OBJECT: ty.ClassVar[str] = "testRunStart"

    name: str = dc.field(
        metadata={"spec_field": "name"},
    )

    version: str = dc.field(
        metadata={"spec_field": "version"},
    )

    command_line: str = dc.field(
        # TODO: remove default
        default="",
        metadata={"spec_field": "commandLine"},
    )

    parameters: dict[str, ty.Any] = dc.field(
        # TODO: remove default
        default_factory=dict,
        metadata={"spec_field": "parameters"},
    )

    # TODO: is still a list?
    dut_info: list[DutInfo] = dc.field(
        # TODO: remove default
        default_factory=list,
        metadata={"spec_field": "dutInfo"},
    )


class TestStatus(Enum):
    COMPLETE = "COMPLETE"
    ERROR = "ERROR"
    SKIP = "SKIP"


class TestResult(Enum):
    PASS = "PASS"
    FAIL = "FAIL"
    NOT_APPLICABLE = "NOT_APPLICABLE"


@dc.dataclass
class RunEnd:
    SPEC_OBJECT: ty.ClassVar[str] = "testRunEnd"

    status: TestStatus = dc.field(
        metadata={
            "spec_field": "status",
            "formatter": format_enum,
        },
    )

    result: TestResult = dc.field(
        metadata={
            "spec_field": "result",
            "formatter": format_enum,
        },
    )


@dc.dataclass
class RunArtifact:
    SPEC_OBJECT: ty.ClassVar[str] = "testRunArtifact"

    impl: RunStart | RunEnd | Log | Error


@dc.dataclass
class StepStart:
    SPEC_OBJECT: ty.ClassVar[str] = "testStepStart"

    name: str = dc.field(
        metadata={"spec_field": "name"},
    )


@dc.dataclass
class StepEnd:
    SPEC_OBJECT: ty.ClassVar[str] = "testStepEnd"

    status: TestStatus = dc.field(
        metadata={
            "spec_field": "status",
            "formatter": format_enum,
        },
    )


@dc.dataclass
class StepArtifact:
    SPEC_OBJECT: ty.ClassVar[str] = "testStepArtifact"

    id: str = dc.field(
        metadata={"spec_field": "testStepId"},
    )

    # TODO: extension
    impl: StepStart | StepEnd | Measurement | MeasurementSeriesType | Diagnosis | Log | Error | File


RootArtifactType = SchemaVersion | RunArtifact | StepArtifact


@dc.dataclass
class Root:
    impl: RootArtifactType

    sequence_number: int = dc.field(
        metadata={"spec_field": "sequenceNumber"},
    )

    timestamp: float = dc.field(
        metadata={
            "spec_field": "timestamp",
            "formatter": format_timestamp,
        },
    )
