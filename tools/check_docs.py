#!/usr/bin/env python3
"""Check that every internal link in the docs site resolves to a real file.

Usage: python tools/check_docs.py  (run from the repository root)

Skips external URLs and the "@/..." placeholders in the shared header and
footer fragments, which components.js rewrites to docs-root-relative paths
at load time.
"""

import os
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"


def main() -> int:
    pages = {p.relative_to(DOCS).as_posix() for p in DOCS.rglob("*.html")}
    assets = {
        p.relative_to(DOCS).as_posix()
        for p in (DOCS / "assets").rglob("*")
        if p.is_file()
    }
    known = pages | assets | {"architecture.md"}
    bad = []
    for page in sorted(DOCS.rglob("*.html")):
        text = page.read_text(encoding="utf-8")
        for link in re.findall(r'(?:href|src)="([^"#{}]+?)(?:[#?][^"]*)?"', text):
            if link.startswith("@/") or re.match(r"[a-zA-Z][a-zA-Z0-9+.-]*:|^//", link):
                continue
            target = os.path.normpath(
                os.path.join(page.parent.relative_to(DOCS).as_posix(), link)
            )
            if target not in known:
                bad.append(f"{page.relative_to(DOCS)} -> {link}")
    if bad:
        print("broken docs links:\n" + "\n".join(bad))
        return 1
    print(f"checked {len(pages)} pages, all internal links resolve")
    return 0


if __name__ == "__main__":
    sys.exit(main())
