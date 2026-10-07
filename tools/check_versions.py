#!/usr/bin/env python3
"""Check that the release version is identical in every file that declares it.

Usage: python tools/check_versions.py  (run from the repository root)

The canonical version lives in python/orderly_chaos/_version.py; the C++
header, the Python/PEP 517 metadata, the Bazel module, and the docs header
must all agree with it.
"""

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent


def main() -> int:
    version = re.search(
        r'__version__ = "([^"]+)"',
        (ROOT / "python" / "orderly_chaos" / "_version.py").read_text(),
    ).group(1)
    checks = {
        "include/orderly_chaos/version.hpp": f'#define ORDERLY_CHAOS_VERSION_STRING "{version}"',
        "pyproject.toml": f'version = "{version}"',
        "MODULE.bazel": f'version = "{version}"',
        "docs/components/header.html": f"<small>v{version}</small>",
    }
    failed = False
    for relative, expected in checks.items():
        text = (ROOT / relative).read_text(encoding="utf-8")
        if expected not in text:
            print(f"{relative}: expected {expected!r}")
            failed = True
    if failed:
        return 1
    print(f"version {version} consistent across {len(checks) + 1} files")
    return 0


if __name__ == "__main__":
    sys.exit(main())
