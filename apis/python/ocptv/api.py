import inspect
import typing as ty

_RT = ty.TypeVar("_RT")


def export_api(ctor: ty.Callable[..., _RT]) -> ty.Callable[..., _RT]:
    """
    This decorator is functionally a no-op. It's just a marker to show lib maintainers
    that a specific method or class is/should be part of the public exports for the lib.
    The semantic of "public exports" here means that the user may create instances
    of these types directly. Most other public types are returned by lib code due to
    them containing private impl details.

    If applied to a class type, a suffix will be appended to its docstring to specify
    that it is a public export and can be directly instantiated by user code.

    All public export items must be re-exported in __init__.py at top module level.
    """

    if inspect.isclass(ctor):
        ctor.__doc__ = (
            f"{ctor.__doc__}\nThis type can be instantiated by user code directly."
        )
    return ctor
