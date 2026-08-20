"""Orderly Chaos: A Non-Consensual Matching Engine for Unruly Capital."""
from .orderly_chaos import LimitOrderBook


# explicitly define the outward facing API of this package
__all__ = [LimitOrderBook.__name__]
