import typing as ty

from .objects import (
    PlatformInfo as PlatformInfoSpec,
    SoftwareInfo as SoftwareInfoSpec,
    SoftwareType,
    HardwareInfo as HardwareInfoSpec,
    DutInfo,
    Subcomponent as SubcomponentSpec,
    SubcomponentType,
    Metadata,
)


class PlatformInfo:
    def __init__(self, info_tag: str):
        self._spec_object = PlatformInfoSpec(info=info_tag)

    def to_spec(self) -> PlatformInfoSpec:
        return self._spec_object


class SoftwareInfo:
    def __init__(
        self,
        id: str,
        name: str,
        type: ty.Optional[SoftwareType] = None,
        version: ty.Optional[str] = None,
        revision: ty.Optional[str] = None,
        computer_system: ty.Optional[str] = None,
    ):
        self._spec_object = SoftwareInfoSpec(
            id=id,
            name=name,
            version=version,
            revision=revision,
            type=type,
            computer_system=computer_system,
        )

    def to_spec(self) -> SoftwareInfoSpec:
        return self._spec_object


class HardwareInfo:
    def __init__(
        self,
        id: str,
        name: str,
        version: ty.Optional[str] = None,
        revision: ty.Optional[str] = None,
        location: ty.Optional[str] = None,
        serial_no: ty.Optional[str] = None,
        part_no: ty.Optional[str] = None,
        manufacturer: ty.Optional[str] = None,
        manufacturer_part_no: ty.Optional[str] = None,
        odata_id: ty.Optional[str] = None,
        computer_system: ty.Optional[str] = None,
        manager: ty.Optional[str] = None,
    ):
        self._spec_object = HardwareInfoSpec(
            id=id,
            name=name,
            version=version,
            revision=revision,
            location=location,
            serial_no=serial_no,
            part_no=part_no,
            manufacturer=manufacturer,
            manufacturer_part_no=manufacturer_part_no,
            odata_id=odata_id,
            computer_system=computer_system,
            manager=manager,
        )

    # TODO: properties?

    def to_spec(self) -> HardwareInfoSpec:
        return self._spec_object


class Dut:
    def __init__(
        self,
        id: str,
        name: ty.Optional[str] = None,
        metadata: ty.Optional[Metadata] = None,
    ):
        self._id = id
        self._name = name
        self._metadata = metadata

        self._platform_infos: list[PlatformInfo] = []
        self._software_infos: list[SoftwareInfo] = []
        self._hardware_infos: list[HardwareInfo] = []

    def add_platform_info(self, info_tag: str) -> PlatformInfo:
        info = PlatformInfo(info_tag=info_tag)
        self._platform_infos.append(info)
        return info

    def add_software_info(
        self,
        name: str,
        type: ty.Optional[SoftwareType] = None,
        version: ty.Optional[str] = None,
        revision: ty.Optional[str] = None,
        computer_system: ty.Optional[str] = None,
    ) -> SoftwareInfo:
        # TODO: arbitrary id derivation
        info = SoftwareInfo(
            id="{}_{}".format(self._id, len(self._software_infos)),
            name=name,
            type=type,
            version=version,
            revision=revision,
            computer_system=computer_system,
        )
        self._software_infos.append(info)
        return info

    def add_hardware_info(
        self,
        name: str,
        version: ty.Optional[str] = None,
        revision: ty.Optional[str] = None,
        location: ty.Optional[str] = None,
        serial_no: ty.Optional[str] = None,
        part_no: ty.Optional[str] = None,
        manufacturer: ty.Optional[str] = None,
        manufacturer_part_no: ty.Optional[str] = None,
        odata_id: ty.Optional[str] = None,
        computer_system: ty.Optional[str] = None,
        manager: ty.Optional[str] = None,
    ) -> HardwareInfo:
        info = HardwareInfo(
            id="{}_{}".format(self._id, len(self._hardware_infos)),
            name=name,
            version=version,
            revision=revision,
            location=location,
            serial_no=serial_no,
            part_no=part_no,
            manufacturer=manufacturer,
            manufacturer_part_no=manufacturer_part_no,
            odata_id=odata_id,
            computer_system=computer_system,
            manager=manager,
        )
        self._hardware_infos.append(info)
        return info

    def to_spec(self) -> DutInfo:
        return DutInfo(
            id=self._id,
            name=self._name,
            platform_infos=[x.to_spec() for x in self._platform_infos],
            software_infos=[x.to_spec() for x in self._software_infos],
            hardware_infos=[x.to_spec() for x in self._hardware_infos],
            metadata=self._metadata,
        )


# Following object is a proxy type so we get future flexibility, avoiding the usage
# of the low-level models.
class Subcomponent:
    def __init__(
        self,
        *,
        name: str,
        type: ty.Optional[SubcomponentType] = None,
        location: ty.Optional[str] = None,
        version: ty.Optional[str] = None,
        revision: ty.Optional[str] = None
    ):
        self._spec_object = SubcomponentSpec(
            type=type,
            name=name,
            location=location,
            version=version,
            revision=revision,
        )

    def to_spec(self) -> SubcomponentSpec:
        return self._spec_object
