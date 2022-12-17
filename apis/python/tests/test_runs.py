import pytest
import typing as ty

import ocptv
from ocptv import LogSeverity, TestStatus, TestResult, DiagnosisType
from ocptv.output import JSON

TestStatus.__test__ = False  # type: ignore
TestResult.__test__ = False  # type: ignore

from .mocks import MockWriter, assert_json


@pytest.fixture
def writer():
    w = MockWriter()
    ocptv.configOutput(w)
    return w


def test_simple_run(writer: MockWriter):
    run = ocptv.TestRun(
        name="test",
        version="1.0",
        command_line="cl",
        parameters={"param": "test"},
    )
    run.start()
    run.end(status=TestStatus.COMPLETE, result=TestResult.PASS)

    assert len(writer.lines) == 3
    assert_json(
        writer.lines[1],
        {
            "testRunArtifact": {
                "testRunStart": {
                    "name": "test",
                    "version": "1.0",
                    "commandLine": "cl",
                    "parameters": {
                        "param": "test",
                    },
                    "dutInfo": [],
                },
            },
            "sequenceNumber": 1,
        },
    )

    assert_json(
        writer.lines[2],
        {
            "testRunArtifact": {
                "testRunEnd": {
                    "status": "COMPLETE",
                    "result": "PASS",
                },
            },
            "sequenceNumber": 2,
        },
    )


def test_run_scope(writer: MockWriter):
    run = ocptv.TestRun(name="test", version="1.0")
    with run.scope():
        pass

    assert len(writer.lines) == 3
    assert_json(
        writer.lines[2],
        {
            "testRunArtifact": {
                "testRunEnd": {
                    "status": "COMPLETE",
                    "result": "PASS",
                },
            },
            "sequenceNumber": 2,
        },
    )


def test_run_skip_by_exception(writer: MockWriter):
    run = ocptv.TestRun(name="run_skip", version="1.0")
    with run.scope():
        raise ocptv.TestRunError(
            status=TestStatus.SKIP, result=TestResult.NOT_APPLICABLE
        )

    assert len(writer.lines) == 3
    assert_json(
        writer.lines[2],
        {
            "testRunArtifact": {
                "testRunEnd": {
                    "status": "SKIP",
                    "result": "NOT_APPLICABLE",
                },
            },
            "sequenceNumber": 2,
        },
    )


def test_run_with_diagnosis(writer: MockWriter):
    class Verdict:
        PASS = "pass-default"

    run = ocptv.TestRun(name="run_with_diagnosis", version="1.0")
    with run.scope():
        run.add_log(LogSeverity.INFO, "run info")
        step = run.add_step("step0")
        with step.scope():
            step.add_diagnosis(DiagnosisType.PASS, verdict=Verdict.PASS)

    assert len(writer.lines) == 7
    assert "schemaVersion" in writer.decoded_obj(0)

    assert "testRunArtifact" in writer.decoded_obj(1)
    artifact = ty.cast(dict[str, JSON], writer.decoded_obj(1)["testRunArtifact"])
    assert "testRunStart" in artifact

    assert "testRunArtifact" in writer.decoded_obj(2)
    artifact = ty.cast(dict[str, JSON], writer.decoded_obj(2)["testRunArtifact"])
    assert "log" in artifact

    assert_json(
        writer.lines[3],
        {
            "testStepArtifact": {
                "testStepStart": {
                    "name": "step0",
                },
                "testStepId": "0",
            },
            "sequenceNumber": 3,
        },
    )

    assert_json(
        writer.lines[4],
        {
            "testStepArtifact": {
                "diagnosis": {
                    "type": "PASS",
                    "verdict": Verdict.PASS,
                },
                "testStepId": "0",
            },
            "sequenceNumber": 4,
        },
    )

    assert_json(
        writer.lines[5],
        {
            "testStepArtifact": {
                "testStepEnd": {
                    "status": "COMPLETE",
                },
                "testStepId": "0",
            },
            "sequenceNumber": 5,
        },
    )

    assert "testRunArtifact" in writer.decoded_obj(6)
    artifact = ty.cast(dict[str, JSON], writer.decoded_obj(6)["testRunArtifact"])
    assert "testRunEnd" in artifact


def test_run_can_error_before_start(writer: MockWriter):
    class Symptom:
        TEST_SYMPTOM = "test-symptom"

    run = ocptv.TestRun(name="test", version="1.0")
    run.add_error(symptom=Symptom.TEST_SYMPTOM)

    assert len(writer.lines), 2
    assert_json(
        writer.lines[1],
        {
            "testRunArtifact": {
                "error": {
                    "symptom": Symptom.TEST_SYMPTOM,
                    "softwareInfoIds": [],
                },
            },
            "sequenceNumber": 1,
        },
    )


def test_run_can_error(writer: MockWriter):
    class Symptom:
        TEST_SYMPTOM = "test-symptom"

    run = ocptv.TestRun(name="test", version="1.0")
    with run.scope():
        run.add_error(symptom=Symptom.TEST_SYMPTOM)

    assert len(writer.lines), 4
    assert_json(
        writer.lines[2],
        {
            "testRunArtifact": {
                "error": {
                    "symptom": Symptom.TEST_SYMPTOM,
                    "softwareInfoIds": [],
                },
            },
            "sequenceNumber": 2,
        },
    )


def test_step_can_error(writer: MockWriter):
    class Symptom:
        TEST_SYMPTOM = "test-symptom"

    run = ocptv.TestRun(name="test", version="1.0")
    with run.scope():
        step = run.add_step("step0")
        with step.scope():
            step.add_error(symptom=Symptom.TEST_SYMPTOM)

    assert len(writer.lines), 7
    assert_json(
        writer.lines[3],
        {
            "testStepArtifact": {
                "error": {
                    "symptom": Symptom.TEST_SYMPTOM,
                    "softwareInfoIds": [],
                },
                "testStepId": "0",
            },
            "sequenceNumber": 3,
        },
    )


def test_step_produces_files(writer: MockWriter):
    run = ocptv.TestRun(name="test", version="1.0")
    with run.scope():
        step = run.add_step("step0")
        with step.scope():
            step.add_file(
                name="device_info.csv",
                uri="file:///root/device_info.csv",
            )

        step = run.add_step("step1")
        with step.scope():
            meta = ocptv.Metadata()
            meta["k"] = "v"

            step.add_file(
                name="file_with_meta.txt",
                uri="ftp://file",
                metadata=meta,
            )

    assert len(writer.lines) == 9
    assert_json(
        writer.lines[3],
        {
            "testStepArtifact": {
                "file": {
                    "displayName": "device_info.csv",
                    "uri": "file:///root/device_info.csv",
                    "isSnapshot": True,
                },
                "testStepId": "0",
            },
            "sequenceNumber": 3,
        },
    )
    assert_json(
        writer.lines[6],
        {
            "testStepArtifact": {
                "file": {
                    "displayName": "file_with_meta.txt",
                    "uri": "ftp://file",
                    "isSnapshot": True,
                    "metadata": {
                        "k": "v",
                    },
                },
                "testStepId": "1",
            },
            "sequenceNumber": 6,
        },
    )


def test_step_produces_simple_measurements(writer: MockWriter):
    run = ocptv.TestRun(name="test", version="1.0")
    with run.scope():
        step = run.add_step("step0")
        with step.scope():
            step.add_measurement(name="fan_speed", value="1200", unit="rpm")
            step.add_measurement(name="temperature", value=42.5)
            step.add_measurement(name="fan_spinning", value=["yes", "no"])

    assert len(writer.lines) == 8
    assert_json(
        writer.lines[3],
        {
            "testStepArtifact": {
                "measurement": {
                    "name": "fan_speed",
                    "value": "1200",
                    "unit": "rpm",
                },
                "testStepId": "0",
            },
            "sequenceNumber": 3,
        },
    )

    assert_json(
        writer.lines[4],
        {
            "testStepArtifact": {
                "measurement": {
                    "name": "temperature",
                    "value": 42.5,
                },
                "testStepId": "0",
            },
            "sequenceNumber": 4,
        },
    )

    assert_json(
        writer.lines[5],
        {
            "testStepArtifact": {
                "measurement": {
                    "name": "fan_spinning",
                    "value": ["yes", "no"],
                },
                "testStepId": "0",
            },
            "sequenceNumber": 5,
        },
    )
