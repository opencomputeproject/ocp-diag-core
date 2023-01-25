import logging
import sys
import threading
import time
import typing as ty

from pathlib import Path

sys.path.append(str(Path(__file__).resolve().parent.parent))

import ocptv
from ocptv import (
    DiagnosisType,
    LogSeverity,
    TestResult,
    TestStatus,
    ValidatorType,
    SubcomponentType,
    SoftwareType,
)
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
    run = ocptv.TestRun(name="no with", version="1.0", parameters={"param": "test"})
    run.start(dut=ocptv.Dut(id="dut0"))

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
    run = ocptv.TestRun(name="run_skip", version="1.0", parameters={"param": "test"})
    with run.scope(dut=ocptv.Dut(id="dut0")):
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
    with run.scope(dut=ocptv.Dut(id="dut0")):
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
        with run.scope(dut=ocptv.Dut(id="dut0")):
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
    with run.scope(dut=ocptv.Dut(id="dut0")):
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
            if level == logging.DEBUG:
                return LogSeverity.DEBUG
            if level == logging.WARN:
                return LogSeverity.WARNING
            if level == logging.ERROR:
                return LogSeverity.ERROR
            if level == logging.FATAL:
                return LogSeverity.FATAL
            return LogSeverity.INFO

    run = ocptv.TestRun(name="run_with_diagnosis", version="1.0")

    log = logging.getLogger("ocptv")
    log.addHandler(Handler(run))
    log.setLevel(logging.DEBUG)
    log.propagate = False

    try:
        with run.scope(dut=ocptv.Dut(id="dut0")):
            log.info("ocp log thru python logger")
            log.debug("debug log sample")
            log.warning("warn level here")
    finally:
        ocptv.configOutput(StdoutWriter())
        log.removeHandler(log.handlers[-1])


@banner
def demo_error_while_gathering_duts():
    class Symptom:
        # str consts for error classifier
        NO_DUT = "no-dut"

    run = ocptv.TestRun(name="test", version="1.0")
    run.add_error(
        symptom=Symptom.NO_DUT,
        message="could not find any valid DUTs",
    )


@banner
def demo_create_file_during_step():
    run = ocptv.TestRun(name="test", version="1.0")
    with run.scope(dut=ocptv.Dut(id="dut0")):
        step = run.add_step("step0")
        with step.scope():
            step.add_file(
                name="device_info.csv",
                uri="file:///root/device_info.csv",
            )

            meta = ocptv.Metadata()
            meta["k"] = "v"
            step.add_file(
                name="file_with_meta.txt",
                uri="ftp://file",
                metadata=meta,
            )


@banner
def demo_create_measurement_simple():
    run = ocptv.TestRun(name="test", version="1.0")
    with run.scope(dut=ocptv.Dut(id="dut0")):
        step = run.add_step("step0")
        with step.scope():
            step.add_measurement(name="fan_speed", value="1200", unit="rpm")
            step.add_measurement(name="temperature", value=42.5)


@banner
def demo_create_measurement_series():
    """
    Show various patterns of time measurement series.
    Step0 has a single series, manually ended.
    Step1 has a single series but using a scope, so series ends automatically.
    Step2 shows multiple measurement interspersed series, they can be concurrent.
    """
    run = ocptv.TestRun(name="test", version="1.0")
    with run.scope(dut=ocptv.Dut(id="dut0")):
        step0 = run.add_step("step0")
        with step0.scope():
            fan_speed = step0.start_measurement_series(name="fan_speed", unit="rpm")
            fan_speed.add_measurement(value=1000)
            fan_speed.add_measurement(value=1200)
            fan_speed.add_measurement(value=1500)
            fan_speed.end()

        step1 = run.add_step("step1")
        with step1.scope():
            temp0 = step1.start_measurement_series(name="temp0", unit="C")
            with temp0.scope():
                temp0.add_measurement(value=42, timestamp=time.time() - 2)
                temp0.add_measurement(value=43)

        step2 = run.add_step("step2")
        with step2.scope():
            s0 = step2.start_measurement_series(name="freq0", unit="ghz")
            s1 = step2.start_measurement_series(name="freq1", unit="ghz")

            s0.add_measurement(value=1.0)
            s1.add_measurement(value=2.0)
            s0.add_measurement(value=1.2)
            s0.end()
            s1.end()


@banner
def demo_create_measurements_with_validators():
    run = ocptv.TestRun(name="test", version="1.0")
    with run.scope(dut=ocptv.Dut(id="dut0")):
        step = run.add_step("step0")
        with step.scope():
            step.add_measurement(
                name="temp",
                value=40,
                validators=[
                    ocptv.Validator(
                        type=ValidatorType.GREATER_THAN,
                        value=30,
                        name="gt_30",
                    )
                ],
            )

            fan_speed = step.start_measurement_series(
                name="fan_speed",
                unit="rpm",
                validators=[
                    ocptv.Validator(
                        type=ValidatorType.LESS_THAN_OR_EQUAL,
                        value=3000,
                    )
                ],
            )
            with fan_speed.scope():
                fan_speed.add_measurement(value=1000)


@banner
def demo_run_error_with_dut():
    dut = ocptv.Dut(id="dut0", name="dut0.server.net")
    bmc_software = dut.add_software_info(
        name="bmc",
        type=SoftwareType.FIRMWARE,
        version="2.5",
    )

    run = ocptv.TestRun(name="test", version="1.0")
    with run.scope(dut=dut):
        run.add_error(
            symptom="power-fail",
            software_infos=[bmc_software],
        )


@banner
def demo_create_measurements_with_subcomponent():
    dut = ocptv.Dut(id="dut0", name="dut0.server.net")
    dut.add_platform_info("memory-optimized")
    dut.add_software_info(
        name="bmc0",
        type=SoftwareType.FIRMWARE,
        version="10",
        revision="11",
        computer_system="primary_node",
    )
    ram0_hardware = dut.add_hardware_info(
        name="ram0",
        version="1",
        revision="2",
        location="MB/DIMM_A1",
        serial_no="HMA2022029281901",
        part_no="P03052-091",
        manufacturer="hynix",
        manufacturer_part_no="HMA84GR7AFR4N-VK",
        odata_id="/redfish/v1/Systems/System.Embedded.1/Memory/DIMMSLOTA1",
        computer_system="primary_node",
        manager="bmc0",
    )

    run = ocptv.TestRun(name="test", version="1.0")
    with run.scope(dut=dut):
        step = run.add_step("step0")
        with step.scope():
            step.add_measurement(
                name="temp0",
                value=100.5,
                unit="F",
                hardware_info=ram0_hardware,
                subcomponent=ocptv.Subcomponent(name="chip0"),
            )

            chip1_temp = step.start_measurement_series(
                name="temp1",
                unit="C",
                hardware_info=ram0_hardware,
                subcomponent=ocptv.Subcomponent(
                    name="chip1",
                    location="U11",
                    version="1",
                    revision="1",
                    type=SubcomponentType.UNSPECIFIED,
                ),
            )
            with chip1_temp.scope():
                chip1_temp.add_measurement(value=79)


@banner
def demo_step_extension():
    run = ocptv.TestRun(name="test", version="1.0")
    with run.scope(dut=ocptv.Dut(id="dut0")):
        step = run.add_step("step0")
        with step.scope():
            step.add_extension(
                name="simple",
                content="extension_identifier",
            )

            step.add_extension(
                name="complex",
                content={
                    "@type": "DemoExtension",
                    "field": "demo",
                    "subtypes": [1, 42],
                },
            )


if __name__ == "__main__":
    demo_no_contexts()
    demo_context_run_skip()
    demo_context_step_fail()
    demo_custom_writer()
    demo_diagnosis()
    demo_python_logging_io()
    demo_error_while_gathering_duts()
    demo_create_file_during_step()
    demo_create_measurement_simple()
    demo_create_measurement_series()
    demo_create_measurements_with_validators()
    demo_run_error_with_dut()
    demo_create_measurements_with_subcomponent()
    demo_step_extension()
