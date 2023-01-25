import json

from ocptv.output import Writer, JSON


class MockWriter(Writer):
    def __init__(self):
        self.lines: list[str] = []

    def write(self, buffer: str):
        self.lines.append(buffer)

    def decoded_obj(self, index: int) -> dict[str, JSON]:
        """Decode an expected object from given output index"""
        return json.loads(self.lines[index])


def assert_json(line: str, expected: JSON):
    actual = json.loads(line)

    # remove timestamps, cant easily compare
    if isinstance(actual, dict) and "timestamp" in actual:
        del actual["timestamp"]

    if isinstance(expected, dict) and "timestamp" in expected:
        del expected["timestamp"]

    assert actual == expected
