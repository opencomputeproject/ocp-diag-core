from datetime import datetime
from enum import Enum


def format_enum(variant: Enum) -> str:
    """Format an enum by variant value"""
    return "{}".format(variant.value)


def format_timestamp(ts: float) -> str:
    """Format an unix timestamp per spec"""
    dt = datetime.fromtimestamp(ts)
    return dt.astimezone().isoformat()
