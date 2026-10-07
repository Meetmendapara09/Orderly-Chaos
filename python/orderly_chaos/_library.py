"""Locate, load, and declare the native shared library (the C API)."""

import ctypes
import glob
import os
import sys
import threading
from typing import List, Optional

from ._version import __version__
from .errors import LibraryError

#: environment variable that overrides the library location
LIBRARY_ENV_VAR = "ORDERLY_CHAOS_LIBRARY"

c_book_p = ctypes.c_void_p
c_side = ctypes.c_int32
c_status = ctypes.c_int32


class OcOrder(ctypes.Structure):
    """Mirror of ``oc_order``."""

    _fields_ = [
        ("order_id", ctypes.c_uint64),
        ("price", ctypes.c_uint64),
        ("quantity", ctypes.c_uint32),
        ("side", c_side),
    ]


class OcTrade(ctypes.Structure):
    """Mirror of ``oc_trade``."""

    _fields_ = [
        ("taker_id", ctypes.c_uint64),
        ("maker_id", ctypes.c_uint64),
        ("price", ctypes.c_uint64),
        ("quantity", ctypes.c_uint32),
        ("maker_remaining", ctypes.c_uint32),
        ("taker_side", c_side),
    ]


class OcLevel(ctypes.Structure):
    """Mirror of ``oc_level``."""

    _fields_ = [
        ("price", ctypes.c_uint64),
        ("volume", ctypes.c_uint64),
        ("count", ctypes.c_uint64),
    ]


#: the C trade callback type: void (*)(const oc_trade*, void*)
TradeCallback = ctypes.CFUNCTYPE(None, ctypes.POINTER(OcTrade), ctypes.c_void_p)

# name: (restype, argtypes)
_PROTOTYPES = {
    "oc_version": (ctypes.c_char_p, []),
    "oc_status_string": (ctypes.c_char_p, [c_status]),
    "oc_book_new": (c_book_p, []),
    "oc_book_free": (None, [c_book_p]),
    "oc_book_clear": (c_status, [c_book_p]),
    "oc_book_set_trade_callback": (c_status, [c_book_p, TradeCallback, ctypes.c_void_p]),
    "oc_book_limit": (
        c_status,
        [c_book_p, c_side, ctypes.c_uint64, ctypes.c_uint32, ctypes.c_uint64, ctypes.POINTER(ctypes.c_uint32)],
    ),
    "oc_book_market": (
        c_status,
        [c_book_p, c_side, ctypes.c_uint64, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)],
    ),
    "oc_book_cancel": (c_status, [c_book_p, ctypes.c_uint64]),
    "oc_book_reduce": (c_status, [c_book_p, ctypes.c_uint64, ctypes.c_uint32]),
    "oc_book_has": (ctypes.c_int, [c_book_p, ctypes.c_uint64]),
    "oc_book_get": (c_status, [c_book_p, ctypes.c_uint64, ctypes.POINTER(OcOrder)]),
    "oc_book_best_price": (ctypes.c_uint64, [c_book_p, c_side]),
    "oc_book_mid_price": (ctypes.c_uint64, [c_book_p]),
    "oc_book_best_volume": (ctypes.c_uint64, [c_book_p, c_side]),
    "oc_book_volume": (ctypes.c_uint64, [c_book_p]),
    "oc_book_side_volume": (ctypes.c_uint64, [c_book_p, c_side]),
    "oc_book_volume_at": (ctypes.c_uint64, [c_book_p, ctypes.c_uint64]),
    "oc_book_side_volume_at": (ctypes.c_uint64, [c_book_p, c_side, ctypes.c_uint64]),
    "oc_book_count": (ctypes.c_uint64, [c_book_p]),
    "oc_book_side_count": (ctypes.c_uint64, [c_book_p, c_side]),
    "oc_book_count_at": (ctypes.c_uint64, [c_book_p, ctypes.c_uint64]),
    "oc_book_depth": (ctypes.c_size_t, [c_book_p, c_side, ctypes.POINTER(OcLevel), ctypes.c_size_t]),
}


def _extensions() -> List[str]:
    """Return the shared library file extensions to search, in order."""
    if sys.platform == "win32":
        return ["dll", "pyd"]  # Bazel emits .dll, setuptools emits .pyd
    if sys.platform == "darwin":
        return ["dylib", "so"]  # Bazel emits .dylib, setuptools emits .so
    return ["so"]


def candidate_paths() -> List[str]:
    """Return the paths that will be tried when loading the library."""
    override = os.environ.get(LIBRARY_ENV_VAR)
    if override:
        return [override]
    directory = os.path.dirname(os.path.abspath(__file__))
    paths: List[str] = []
    for extension in _extensions():
        paths.extend(sorted(glob.glob(os.path.join(directory, f"lib_orderly_chaos*.{extension}"))))
    return paths


def _load() -> ctypes.PyDLL:
    paths = candidate_paths()
    if not paths:
        patterns = ", ".join(f"lib_orderly_chaos*.{e}" for e in _extensions())
        raise LibraryError(
            f"the Orderly Chaos native library was not found ({patterns} next to "
            f"{os.path.dirname(os.path.abspath(__file__))}). Install the package with "
            f"`pip install .`, run `make python-lib`, or set {LIBRARY_ENV_VAR} to the library path."
        )
    errors = []
    for path in paths:
        try:
            # PyDLL keeps the GIL held during calls, so concurrent Python
            # threads can never enter the (non-thread-safe) book at once.
            library = ctypes.PyDLL(path)
        except OSError as error:
            errors.append(f"{path}: {error}")
            continue
        _declare(library, path)
        return library
    raise LibraryError("failed to load the Orderly Chaos native library:\n  " + "\n  ".join(errors))


def _declare(library: ctypes.PyDLL, path: str) -> None:
    """Declare argument/return types and verify the library version."""
    for name, (restype, argtypes) in _PROTOTYPES.items():
        try:
            function = getattr(library, name)
        except AttributeError:
            raise LibraryError(
                f"{path} does not export {name}; it was built from an incompatible "
                f"version of Orderly Chaos. Rebuild it (e.g. `make python-lib`)."
            ) from None
        function.restype = restype
        function.argtypes = argtypes
    native = library.oc_version().decode("ascii")
    if native.split(".")[:2] != __version__.split(".")[:2]:
        raise LibraryError(
            f"{path} is version {native} but the Python package is version {__version__}. "
            f"Rebuild the library (e.g. `make python-lib`)."
        )


_lock = threading.Lock()
_library: Optional[ctypes.PyDLL] = None


def library() -> ctypes.PyDLL:
    """Return the loaded native library, loading it on first use.

    Raises:
        LibraryError: if the library cannot be found or is incompatible

    """
    global _library
    if _library is None:
        with _lock:
            if _library is None:
                _library = _load()
    return _library


def native_version() -> str:
    """Return the version string reported by the native library."""
    return library().oc_version().decode("ascii")
