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
from .api import export_api


class PlatformInfo:
    """
    Platform information for the DUT.
    See spec: https://github.com/opencomputeproject/ocp-diag-core/tree/main/json_spec#platforminfo
    """

    def __init__(self, info_tag: str):
        self._spec_object = PlatformInfoSpec(info=info_tag)

    def to_spec(self) -> PlatformInfoSpec:
        """internal usage"""
        return self._spec_object


class SoftwareInfo:
    """
    Software information for a component of the DUT.
    See spec: https://github.com/opencomputeproject/ocp-diag-core/tree/main/json_spec#softwareinfo
    """

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
        """internal usage"""
        return self._spec_object


class HardwareInfo:
    """
    Hardware information for a component of the DUT.
    See spec: https://github.com/opencomputeproject/ocp-diag-core/tree/main/json_spec#hardwareinfo
    """

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

    # TODO(mimir-d): properties?

    def to_spec(self) -> HardwareInfoSpec:
        """internal usage"""
        return self._spec_object


@export_api
class Dut:
    """
    The `Dut` instances model the specific device under test that the output is relative to.

    ref: https://github.com/opencomputeproject/ocp-diag-core/tree/main/json_spec#dutinfo
    """

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
        # TODO(mimir-d): arbitrary id derivation; does this need to be more readable?
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
        """internal usage"""
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
@export_api
class Subcomponent:
    """
    A lower-than-FRU (field replaceable unit) for the DUT hardware.
    See spec: https://github.com/opencomputeproject/ocp-diag-core/tree/main/json_spec#platforminfo
    """

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
        """internal usage"""
        return self._spec_object
