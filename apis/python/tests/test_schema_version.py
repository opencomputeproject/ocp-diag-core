import pytest

import ocptv

from .mocks import MockWriter, assert_json


@pytest.fixture
def writer():
    w = MockWriter()
    ocptv.configOutput(w)
    return w


def test_schema_version_is_emitted(writer: MockWriter):
    run = ocptv.TestRun(name="test", version="1.0")
    run.start(dut=ocptv.Dut(id="test_dut"))

    assert len(writer.lines) == 2
    assert_json(
        writer.lines[0],
        {
            "schemaVersion": {
                "major": ocptv.OCPVersion.VERSION_2_0.value[0],
                "minor": ocptv.OCPVersion.VERSION_2_0.value[1],
            },
            "sequenceNumber": 0,
        },
    )
