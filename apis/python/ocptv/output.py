"""
This module covers the raw output semantics of the OCPTV library.
"""
import time
import threading
from abc import ABC, abstractmethod
import dataclasses as dc
import json
import typing as ty
from enum import Enum

from .objects import ArtifactType, Root, SchemaVersion, RootArtifactType
from .api import export_api


class Writer(ABC):
    """
    Abstract writer interface for the lib. Should be used as a base for
    any output writer implementation (for typing purposes).
    """

    @abstractmethod
    def write(self, buffer: str):
        pass


@export_api
class StdoutWriter(Writer):
    """
    A simple writer that prints the json to stdout.
    """

    def write(self, buffer: str):
        print(buffer)


# module scoped output channel (similar to python logging)
_writer: Writer = StdoutWriter()


@export_api
def configOutput(writer: Writer):
    """
    Configure the output channel for this library.
    """
    global _writer
    _writer = writer


Primitive = ty.Union[float, int, bool, str, None]
JSON = ty.Union[ty.Dict[str, "JSON"], ty.List["JSON"], Primitive]


class ArtifactEmitter:
    """
    Serializes and emits the data on the configured output channel for the lib.
    Uses the low level dataclass models for the spec, but should not be used in user code.
    """

    def __init__(self):
        self._seq = 0
        self._lock = threading.Lock()

    @staticmethod
    def _serialize(artifact: ArtifactType):
        def is_optional(field: ty.Type):
            # type hackery incoming
            # ty.Optional[T] == ty.Union[T, None]
            # since ty.Union[ty.Union[T,U]] = ty.Union[T,U] we can the
            # following transitiveness to check that type is optional
            return field == ty.Optional[field]

        def visit(
            value: ty.Union[ArtifactType, ty.Dict, ty.List, Primitive],
            formatter: ty.Optional[ty.Callable[[ty.Any], str]] = None,
        ) -> JSON:
            # if present, formatter takes precedence over serialization
            if formatter is not None:
                return formatter(value)

            if dc.is_dataclass(value):
                obj: ty.Dict[str, JSON] = {}
                for field in dc.fields(value):
                    val = getattr(value, field.name)

                    if val is None:
                        if not is_optional(field.type):
                            # TODO: fix exception text/type
                            raise RuntimeError("unacceptable none where not optional")
                        continue

                    # spec_field takes precedence over spec_object
                    spec_field: ty.Optional[str] = field.metadata.get(
                        "spec_field", None
                    )
                    spec_object: ty.Optional[str] = getattr(val, "SPEC_OBJECT", None)

                    if spec_field is None and spec_object is None:
                        # TODO: fix error type
                        raise RuntimeError(
                            "internal error, bad object decl: neither spec_field nor spec_object present for {}".format(
                                field.name
                            )
                        )

                    # safety: at least one is not None
                    name: str = ty.cast(str, spec_field or spec_object)
                    formatter = field.metadata.get("formatter", None)
                    obj[name] = visit(val, formatter)
                return obj
            elif isinstance(value, list) or isinstance(value, tuple):
                return [visit(k) for k in value]
            elif isinstance(value, dict):
                return {k: visit(v) for k, v in value.items()}
            elif isinstance(value, (str, float, int, bool, Enum)):
                return value

            raise RuntimeError("dont know how to serialize {}", value)

        return json.dumps(visit(artifact))

    def emit(self, artifact: RootArtifactType):
        if self._seq == 0:
            self._emit_version()

        # this sync point needs to happen before the write
        with self._lock:
            # multiprocess will have an issue here
            self._seq += 1

        root = Root(
            impl=artifact,
            sequence_number=self._seq,
            timestamp=time.time(),
        )
        _writer.write(self._serialize(root))

    def _emit_version(self):
        # use defaults for schema version here, should be set to latest
        root = Root(
            impl=SchemaVersion(),
            sequence_number=self._seq,
            timestamp=time.time(),
        )
        _writer.write(self._serialize(root))
