from datetime import datetime
from enum import Enum
import typing as ty


def format_enum(variant: Enum) -> str:
    """Format an enum by variant name"""
    return "{}".format(variant.name)


def format_timestamp(ts: float) -> str:
    """Format an unix timestamp per spec"""
    dt = datetime.fromtimestamp(ts)
    return dt.astimezone().isoformat()
