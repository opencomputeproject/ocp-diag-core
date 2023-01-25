import pytest
import time
import typing as ty

import ocptv
from ocptv import (
    LogSeverity,
    TestStatus,
    TestResult,
    DiagnosisType,
    ValidatorType,
    SoftwareType,
)
from ocptv.formatter import format_timestamp
from ocptv.output import JSON

# pytest incorrectly identifies these as pytest related
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
    run.start(dut=ocptv.Dut(id="test_dut"))
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
                    "dutInfo": {
                        "dutInfoId": "test_dut",
                        "platformInfos": [],
                        "softwareInfos": [],
                        "hardwareInfos": [],
                    },
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
    with run.scope(dut=ocptv.Dut(id="test_dut")):
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
    with run.scope(dut=ocptv.Dut(id="test_dut")):
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

    dut = ocptv.Dut(id="test_dut")
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

    run = ocptv.TestRun(name="run_with_diagnosis", version="1.0")
    with run.scope(dut=dut):
        run.add_log(LogSeverity.INFO, "run info")
        step = run.add_step("step0")
        with step.scope():
            step.add_diagnosis(
                DiagnosisType.PASS, verdict=Verdict.PASS, hardware_info=ram0_hardware
            )

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
                    "hardwareInfoId": "test_dut_0",
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

    dut = ocptv.Dut(id="test_dut")
    bmc_software = dut.add_software_info(
        name="bmc",
        type=SoftwareType.FIRMWARE,
        version="1.2",
    )

    run = ocptv.TestRun(name="test", version="1.0")
    with run.scope(dut=dut):
        run.add_error(symptom=Symptom.TEST_SYMPTOM, software_infos=[bmc_software])

    assert len(writer.lines), 4
    assert_json(
        writer.lines[2],
        {
            "testRunArtifact": {
                "error": {
                    "symptom": Symptom.TEST_SYMPTOM,
                    "softwareInfoIds": ["test_dut_0"],
                },
            },
            "sequenceNumber": 2,
        },
    )


def test_step_can_error(writer: MockWriter):
    class Symptom:
        TEST_SYMPTOM = "test-symptom"

    dut = ocptv.Dut(id="test_dut")
    bmc_software = dut.add_software_info(
        name="bmc",
        type=SoftwareType.FIRMWARE,
        version="1.2",
    )

    run = ocptv.TestRun(name="test", version="1.0")
    with run.scope(dut=dut):
        step = run.add_step("step0")
        with step.scope():
            step.add_error(symptom=Symptom.TEST_SYMPTOM, software_infos=[bmc_software])

    assert len(writer.lines), 7
    assert_json(
        writer.lines[3],
        {
            "testStepArtifact": {
                "error": {
                    "symptom": Symptom.TEST_SYMPTOM,
                    "softwareInfoIds": ["test_dut_0"],
                },
                "testStepId": "0",
            },
            "sequenceNumber": 3,
        },
    )


def test_step_produces_files(writer: MockWriter):
    run = ocptv.TestRun(name="test", version="1.0")
    with run.scope(dut=ocptv.Dut(id="test_dut")):
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


def test_step_produces_extensions(writer: MockWriter):
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

    assert len(writer.lines) == 7
    assert_json(
        writer.lines[3],
        {
            "testStepArtifact": {
                "extension": {
                    "name": "simple",
                    "content": "extension_identifier",
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
                "extension": {
                    "name": "complex",
                    "content": {
                        "@type": "DemoExtension",
                        "field": "demo",
                        "subtypes": [1, 42],
                    },
                },
                "testStepId": "0",
            },
            "sequenceNumber": 4,
        },
    )


def test_step_produces_simple_measurements(writer: MockWriter):
    run = ocptv.TestRun(name="test", version="1.0")
    with run.scope(dut=ocptv.Dut(id="test_dut")):
        step = run.add_step("step0")
        with step.scope():
            step.add_measurement(name="fan_speed", value="1200", unit="rpm")
            step.add_measurement(name="temperature", value=42.5)

    assert len(writer.lines) == 7
    assert_json(
        writer.lines[3],
        {
            "testStepArtifact": {
                "measurement": {
                    "name": "fan_speed",
                    "value": "1200",
                    "unit": "rpm",
                    "validators": [],
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
                    "validators": [],
                },
                "testStepId": "0",
            },
            "sequenceNumber": 4,
        },
    )


def test_step_produces_measurement_series(writer: MockWriter):
    ts = time.time()

    run = ocptv.TestRun(name="test", version="1.0")
    with run.scope(dut=ocptv.Dut(id="test_dut")):
        step = run.add_step("step0")
        with step.scope():
            fan_speed = step.start_measurement_series(name="fan_speed", unit="rpm")
            with fan_speed.scope():
                fan_speed.add_measurement(value=1200, timestamp=ts)
                fan_speed.add_measurement(value=1500, timestamp=ts)

    assert len(writer.lines) == 9
    assert_json(
        writer.lines[3],
        {
            "testStepArtifact": {
                "measurementSeriesStart": {
                    "name": "fan_speed",
                    "unit": "rpm",
                    "measurementSeriesId": "0_0",
                    "validators": [],
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
                "measurementSeriesElement": {
                    "index": 0,
                    "value": 1200,
                    "timestamp": format_timestamp(ts),
                    "measurementSeriesId": "0_0",
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
                "measurementSeriesElement": {
                    "index": 1,
                    "value": 1500,
                    "timestamp": format_timestamp(ts),
                    "measurementSeriesId": "0_0",
                },
                "testStepId": "0",
            },
            "sequenceNumber": 5,
        },
    )
    assert_json(
        writer.lines[6],
        {
            "testStepArtifact": {
                "measurementSeriesEnd": {
                    "totalCount": 2,
                    "measurementSeriesId": "0_0",
                },
                "testStepId": "0",
            },
            "sequenceNumber": 6,
        },
    )


def test_step_produces_concurrent_measurement_series(writer: MockWriter):
    ts = time.time()

    run = ocptv.TestRun(name="test", version="1.0")
    with run.scope(dut=ocptv.Dut(id="test_dut")):
        step = run.add_step("step0")
        with step.scope():
            fan_speed = step.start_measurement_series(name="fan_speed", unit="rpm")
            fan_speed.add_measurement(value=1200, timestamp=ts)
            temp = step.start_measurement_series(name="temp", unit="C")
            temp.add_measurement(value=42, timestamp=ts)
            fan_speed.add_measurement(value=1500, timestamp=ts)
            temp.end()
            fan_speed.end()

    assert len(writer.lines) == 12

    # fan_speed start
    assert "testStepArtifact" in writer.decoded_obj(3)
    artifact = ty.cast(dict[str, JSON], writer.decoded_obj(3)["testStepArtifact"])
    assert "measurementSeriesStart" in artifact

    # temp start
    assert "testStepArtifact" in writer.decoded_obj(5)
    artifact = ty.cast(dict[str, JSON], writer.decoded_obj(5)["testStepArtifact"])
    assert "measurementSeriesStart" in artifact

    assert_json(
        writer.lines[6],
        {
            "testStepArtifact": {
                "measurementSeriesElement": {
                    "index": 0,
                    "value": 42,
                    "timestamp": format_timestamp(ts),
                    "measurementSeriesId": "0_1",
                },
                "testStepId": "0",
            },
            "sequenceNumber": 6,
        },
    )

    assert_json(
        writer.lines[7],
        {
            "testStepArtifact": {
                "measurementSeriesElement": {
                    "index": 1,
                    "value": 1500,
                    "timestamp": format_timestamp(ts),
                    "measurementSeriesId": "0_0",
                },
                "testStepId": "0",
            },
            "sequenceNumber": 7,
        },
    )

    # temp end
    assert "testStepArtifact" in writer.decoded_obj(8)
    artifact = ty.cast(dict[str, JSON], writer.decoded_obj(8)["testStepArtifact"])
    assert "measurementSeriesEnd" in artifact

    # fan_speed end
    assert "testStepArtifact" in writer.decoded_obj(9)
    artifact = ty.cast(dict[str, JSON], writer.decoded_obj(9)["testStepArtifact"])
    assert "measurementSeriesEnd" in artifact


def test_step_produces_measurements_with_validators(writer: MockWriter):
    run = ocptv.TestRun(name="test", version="1.0")
    with run.scope(dut=ocptv.Dut(id="test_dut")):
        step0 = run.add_step("step0")
        with step0.scope():
            step0.add_measurement(
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

        step1 = run.add_step("step1")
        with step1.scope():
            fan_speed = step1.start_measurement_series(
                name="fan_speed",
                unit="rpm",
                validators=[
                    ocptv.Validator(
                        type=ValidatorType.GREATER_THAN,
                        value=500,
                    ),
                    ocptv.Validator(
                        type=ValidatorType.LESS_THAN_OR_EQUAL,
                        value=3000,
                    ),
                ],
            )
            with fan_speed.scope():
                fan_speed.add_measurement(value=1000)

    assert len(writer.lines) == 11
    assert_json(
        writer.lines[3],
        {
            "testStepArtifact": {
                "measurement": {
                    "name": "temp",
                    "value": 40,
                    "validators": [
                        {
                            "name": "gt_30",
                            "type": "GREATER_THAN",
                            "value": 30,
                        }
                    ],
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
                "measurementSeriesStart": {
                    "name": "fan_speed",
                    "unit": "rpm",
                    "measurementSeriesId": "1_0",
                    "validators": [
                        {
                            "type": "GREATER_THAN",
                            "value": 500,
                        },
                        {
                            "type": "LESS_THAN_OR_EQUAL",
                            "value": 3000,
                        },
                    ],
                },
                "testStepId": "1",
            },
            "sequenceNumber": 6,
        },
    )


def test_step_produces_measurements_with_dut_subcomponent(writer: MockWriter):
    dut = ocptv.Dut(id="test_dut", name="test.server.net")
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

    run = ocptv.TestRun(name="test", version="1.0", command_line="cl")
    with run.scope(dut=dut):
        step = run.add_step("step0")
        with step.scope():
            step.add_measurement(
                name="temp0",
                value=40.2,
                unit="C",
                hardware_info=ram0_hardware,
                subcomponent=ocptv.Subcomponent(name="chip0", location="U1"),
            )

    assert len(writer.lines) == 6
    assert_json(
        writer.lines[1],
        {
            "testRunArtifact": {
                "testRunStart": {
                    "name": "test",
                    "version": "1.0",
                    "commandLine": "cl",
                    "parameters": {},
                    "dutInfo": {
                        "dutInfoId": "test_dut",
                        "name": "test.server.net",
                        "platformInfos": [{"info": "memory-optimized"}],
                        "softwareInfos": [
                            {
                                "softwareInfoId": "test_dut_0",
                                "name": "bmc0",
                                "softwareType": "FIRMWARE",
                                "version": "10",
                                "revision": "11",
                                "computerSystem": "primary_node",
                            }
                        ],
                        "hardwareInfos": [
                            {
                                "hardwareInfoId": "test_dut_0",
                                "name": "ram0",
                                "version": "1",
                                "revision": "2",
                                "location": "MB/DIMM_A1",
                                "serialNumber": "HMA2022029281901",
                                "partNumber": "P03052-091",
                                "manufacturer": "hynix",
                                "manufacturerPartNumber": "HMA84GR7AFR4N-VK",
                                "odataId": "/redfish/v1/Systems/System.Embedded.1/Memory/DIMMSLOTA1",
                                "computerSystem": "primary_node",
                                "manager": "bmc0",
                            }
                        ],
                    },
                },
            },
            "sequenceNumber": 1,
            "timestamp": "<ignored>",
        },
    )

    assert_json(
        writer.lines[3],
        {
            "testStepArtifact": {
                "measurement": {
                    "name": "temp0",
                    "value": 40.2,
                    "unit": "C",
                    "validators": [],
                    "hardwareInfoId": "test_dut_0",
                    "subcomponent": {
                        "name": "chip0",
                        "location": "U1",
                    },
                },
                "testStepId": "0",
            },
            "sequenceNumber": 3,
            "timestamp": "<ignored>",
        },
    )
