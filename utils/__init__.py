from collections.abc import Collection
from typing import TypeVar

T = TypeVar("T")


def first(it: Collection[T]) -> T:
    return next(iter(it))
