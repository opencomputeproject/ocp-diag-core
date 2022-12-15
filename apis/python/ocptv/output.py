"""
This module covers the raw output semantics of the OCPTV library.
"""
import time
import threading
from abc import ABC, abstractmethod

from .objects import TempArtifactModel, OCPVersion


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
_writer = StdoutWriter()


def configOutput(writer: Writer):
    global _writer
    _writer = writer


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
