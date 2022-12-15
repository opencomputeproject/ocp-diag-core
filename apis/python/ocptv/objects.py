"""
Low level object models that are to be serialized as JSON.
NOT PUBLIC API, these are not intended to be used by client code
unless explicitly exported as public in __init__.py
"""
from enum import Enum


class TempArtifactModel:
    def __init__(self, n, v):
        self.name = n
        self.value = v


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
