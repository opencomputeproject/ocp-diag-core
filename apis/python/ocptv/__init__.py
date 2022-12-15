"""
The OCP Test & Validation python api for the output of spec compliant JSON.
"""
__version__ = "0.1.0"
__author__ = "OCP Test & Validation"

# following are public api re-exports to make module usage easier
from .output import configOutput
from .step import TestStep, TestStepError
from .run import TestRun, TestRunError

from .objects import OCPVersion, TestStatus, TestResult, LogSeverity, DiagnosisType
