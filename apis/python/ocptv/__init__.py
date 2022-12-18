"""
The OCP Test & Validation python api for the output of spec compliant JSON.
"""
__version__ = "0.1.0"
__author__ = "OCP Test & Validation"

# the following are the only public api exports
from .step import TestStep, TestStepError
from .run import TestRun, TestRunError
from .measurement import Validator
from .dut import Dut, Subcomponent
from .output import configOutput

from .objects import (
    OCPVersion,
    TestStatus,
    TestResult,
    LogSeverity,
    DiagnosisType,
    ValidatorType,
    SubcomponentType,
    SoftwareType,
    Metadata,
)
