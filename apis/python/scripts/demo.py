import logging
import sys
import threading
import time
import typing as ty

from pathlib import Path

sys.path.append(str(Path(__file__).resolve().parent.parent))

import ocptv
from ocptv import DiagnosisType, LogSeverity, TestResult, TestStatus
from ocptv.output import Writer, StdoutWriter


def banner(f):
    def w():
        print("-" * 80)
        print(f.__name__)
        print("-" * 80)
        f()
        print()

    return w


@banner
def demo_no_contexts():
    """
    Show that a run/step can be manually started and ended
    (but it's safer with context)
    """
    run = ocptv.TestRun(
        name="no with", version="1.0", parameters={"param1": "dog", "param2": "cat"}
    )
    run.start(dut_info=None)

    step = run.add_step("step0")
    step.start()
    step.add_log(LogSeverity.DEBUG, "Some interesting message.")
    step.end(status=TestStatus.COMPLETE)

    run.end(status=TestStatus.COMPLETE, result=TestResult.PASS)


@banner
def demo_context_run_skip():
    """
    Show a context-scoped run that automatically exits the whole func
    because of the marker exception that triggers SKIP outcome.
    """
    run = ocptv.TestRun(
        name="run_skip", version="1.0", parameters={"param1": "dog", "param2": "cat"}
    )
    with run.scope(dut_info=None):
        raise ocptv.TestRunError(
            status=TestStatus.SKIP, result=TestResult.NOT_APPLICABLE
        )


@banner
def demo_context_step_fail():
    """
    Show a scoped run with scoped steps, everything starts at "with" time and
    ends automatically when the block ends (regardless of unhandled exceptions).
    """
    run = ocptv.TestRun(name="step_fail", version="1.0")
    with run.scope(dut_info=None):
        step = run.add_step("step0")
        with step.scope():
            step.add_log(LogSeverity.INFO, "info log")

        step = run.add_step("step1")
        with step.scope():
            # TODO: maybe this should fail the whole run?
            raise ocptv.TestStepError(status=TestStatus.ERROR)


@banner
def demo_custom_writer():
    """
    Showcase parallel running steps outputting to a threadsafe writer on top of
    stdout. The start/end messages get correctly scoped.
    """

    class FileSyncWriter(Writer):
        def __init__(self, file: ty.TextIO):
            self.__file = file
            self.__lock = threading.Lock()

        def write(self, buffer: str):
            with self.__lock:
                print(buffer, file=self.__file)

    ocptv.configOutput(writer=FileSyncWriter(sys.stdout))

    def parallel_step(step: ocptv.TestStep):
        with step.scope():
            for _ in range(5):
                step.add_log(
                    LogSeverity.INFO,
                    f"log from: {step.name}, ts: {time.time()}",
                )
                time.sleep(0.001)

    try:
        run = ocptv.TestRun(name="custom writer", version="1.0")
        with run.scope(dut_info={}):
            threads: list[threading.Thread] = []
            for id in range(4):
                step = run.add_step(f"parallel_step_{id}")
                threads.append(threading.Thread(target=parallel_step, args=(step,)))

            for t in threads:
                t.start()

            for t in threads:
                t.join()
    finally:
        # return to default, useful for rest of demos
        ocptv.configOutput(StdoutWriter())


@banner
def demo_diagnosis():
    """
    Show outputting a diagnosis message for the given step.
    """

    class Verdict:
        # str consts for error classifier
        PASS = "pass-default"

    run = ocptv.TestRun(name="run_with_diagnosis", version="1.0")
    with run.scope(dut_info=None):
        step = run.add_step("step0")
        with step.scope():
            step.add_diagnosis(DiagnosisType.PASS, verdict=Verdict.PASS)


@banner
def demo_python_logging_io():
    """
    Show that we can output to a python logger backed writer (note the pylog prefix)
    and that we can route the standard python logger as input for the ocp output (mapped to run logs).
    This demo is shown as a migration path for code using standard python logging, until
    we have some on-parity functionality (eg. rolling/archiving output writer).
    """

    class LoggingWriter(Writer):
        def __init__(self):
            logging.basicConfig(
                stream=sys.stdout, level=logging.INFO, format="[pylog] %(message)s"
            )

        def write(self, buffer: str):
            logging.info(buffer)

    ocptv.configOutput(writer=LoggingWriter())

    class Handler(logging.StreamHandler):
        def __init__(self, run: ocptv.TestRun):
            super().__init__(None)
            self._run = run

        def emit(self, record: logging.LogRecord):
            self._run.add_log(self._map_level(record.levelno), record.msg)

        @staticmethod
        def _map_level(level: int) -> LogSeverity:
            if level == logging.INFO:
                return LogSeverity.INFO
            if level == logging.DEBUG:
                return LogSeverity.DEBUG
            if level == logging.WARN:
                return LogSeverity.WARNING
            if level == logging.ERROR:
                return LogSeverity.ERROR
            if level == logging.FATAL:
                return LogSeverity.FATAL

    run = ocptv.TestRun(name="run_with_diagnosis", version="1.0")

    log = logging.getLogger("ocptv")
    log.addHandler(Handler(run))
    log.setLevel(logging.DEBUG)
    log.propagate = False

    with run.scope(dut_info=None):
        log.info("ocp log thru python logger")
        log.debug("debug log sample")
        log.warning("warn level here")


if __name__ == "__main__":
    demo_no_contexts()
    demo_context_run_skip()
    demo_context_step_fail()
    demo_custom_writer()
    demo_diagnosis()
    demo_python_logging_io()
