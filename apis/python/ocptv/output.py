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


class Writer(ABC):
    """
    Abstract writer interface for the lib. Should be used as a base for
    any output writer implementation (for typing purposes).
    """

    @abstractmethod
    def write(self, buffer: str):
        pass


class StdoutWriter(Writer):
    def write(self, buffer: str):
        print(buffer)


# module scoped output channel (similar to python logging)
_writer: Writer = StdoutWriter()


def configOutput(writer: Writer):
    global _writer
    _writer = writer


Primitive = float | int | bool | str | None
JSON = dict[str, "JSON"] | list["JSON"] | Primitive


class ArtifactEmitter:
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
            value: ArtifactType | dict | list | Primitive,
            formatter: ty.Optional[ty.Callable[[ty.Any], str]] = None,
        ) -> JSON:
            if dc.is_dataclass(value):
                obj: dict[str, JSON] = {}
                for field in dc.fields(value):
                    val = getattr(value, field.name)

                    if val is None:
                        if not is_optional(field.type):
                            # TODO: fix exception text/type
                            raise RuntimeError("unacceptable none where not optional")
                        continue

                    spec_field: ty.Optional[str] = field.metadata.get(
                        "spec_field", None
                    )
                    spec_object: ty.Optional[str] = getattr(val, "SPEC_OBJECT", None)

                    if spec_field is not None and spec_object is not None:
                        # TODO: fix error type
                        raise RuntimeError("internal error, bad object decl")

                    if spec_field is None and spec_object is None:
                        # TODO: fix error type
                        raise RuntimeError("internal error, bad object decl")

                    # "<invalid>" will never be reached, but keeps linter happy
                    name = spec_field or spec_object or "<invalid>"
                    formatter = field.metadata.get("formatter", None)
                    obj[name] = visit(val, formatter)
                return obj
            elif isinstance(value, list) or isinstance(value, tuple):
                return [visit(k) for k in value]
            elif isinstance(value, dict):
                return {k: visit(v) for k, v in value.items()}
            elif isinstance(value, (str, float, int, bool, Enum)):
                # primitive types get here
                if formatter is not None:
                    return formatter(value)
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
