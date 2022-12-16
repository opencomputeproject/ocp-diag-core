from .objects import DutInfo


class Dut:
    def __str__(self):
        return "dut_info_here"

    @property
    def info(self) -> DutInfo:
        return DutInfo()
