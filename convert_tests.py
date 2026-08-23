#!/usr/bin/env python3
"""Convert Catch2 SCENARIO/GIVEN/WHEN/THEN tests to Google Test.

Catch2 executes each GIVEN in its own run, and each WHEN within a GIVEN
in yet another run (with the GIVEN setup re-executed). To preserve
semantics, this converter emits one TEST per (scenario, given, when)
combination:

    TEST(Suite, scenario__given__when) {
        <scenario-level setup>
        <given-level setup>
        <when body>
    }
"""

import re

BLOCK_OPEN_RE = re.compile(
    r'^(\s*)(GIVEN|AND_WHEN|WHEN|AND_THEN|THEN)\(\s*"(.+?)"\s*\)\s*\{\s*$'
)


def sanitize(name):
    name = re.sub(r"[^a-zA-Z0-9_]", "_", name.strip())
    return re.sub(r"_+", "_", name).strip("_")[:60]


def read_braced(lines, i):
    """lines[i] contains the opening '{' of a block.
    Returns (inner_lines, index_after_closing_brace_line)."""
    depth = 0
    inner = []
    j = i
    while j < len(lines):
        st = lines[j].strip()
        if depth == 0:
            depth += st.count("{")
            j += 1
            continue
        if st == "}":
            depth -= 1
            if depth == 0:
                return inner, j + 1
            inner.append(lines[j])
            j += 1
            continue
        inner.append(lines[j])
        depth += st.count("{") - st.count("}")
        j += 1
    raise RuntimeError("unbalanced braces")


def parse_items(inner):
    """Split block inner lines into items: ('stmt', [lines]) | ('block', kind, desc, inner)."""
    items = []
    i = 0
    pending = []
    while i < len(inner):
        l = inner[i]
        m = BLOCK_OPEN_RE.match(l)
        if m:
            if pending:
                items.append(("stmt", pending))
                pending = []
            block_inner, i = read_braced(inner, i)
            items.append(("block", m.group(2), m.group(3), block_inner))
            continue
        pending.append(l)
        i += 1
    if pending:
        items.append(("stmt", pending))
    return items


def strip_blank(lines):
    out = [l for l in lines if l.strip() != ""]
    return out


def dedent(lines):
    indents = [len(l) - len(l.lstrip()) for l in lines if l.strip()]
    base = min(indents) if indents else 0
    return [l[base:] if len(l) >= base else l.lstrip() for l in lines]


def flatten_then(inner):
    """Flatten THEN/AND_THEN block wrappers into plain statement lines."""
    out = []
    for kind, *rest in parse_items(inner):
        if kind == "stmt":
            out += rest[0]
        else:
            # THEN / AND_THEN / nested block: inline its contents
            out += rest[2]
    return out


def extract_test_name(text):
    raw = re.search(r'SCENARIO\(\s*R"\((.+?)\)\s*(?:"([^"]*)")?\s*\)', text, re.DOTALL)
    if raw:
        if raw.group(2):
            return raw.group(2)
        parts = [p.strip() for p in raw.group(1).strip().split("\n") if p.strip()]
        if parts and parts[0] == "LOB":
            parts = parts[1:]
        return " ".join(parts) if parts else "raw"
    m = re.search(r'SCENARIO\(\s*"([^"]+)"\s*\)', text)
    if m:
        return m.group(1)
    m = re.search(r'TEST_CASE\(\s*"([^"]+)"\s*\)', text)
    if m:
        return m.group(1)
    return "unknown"


def convert_require(line):
    line = re.sub(r"REQUIRE_THROWS\(\s*(.+?)\s*\)", r"EXPECT_ANY_THROW(\1)", line)
    line = re.sub(
        r"REQUIRE_THROWS_AS\(\s*(.+?),\s*(.+?)\s*\)", r"EXPECT_THROW(\1, \2)", line
    )
    line = re.sub(r"REQUIRE_NOTHROW\(\s*(.+?)\s*\)", r"EXPECT_NO_THROW(\1)", line)
    line = re.sub(r"REQUIRE_FALSE\(\s*(.+?)\s*\)", r"EXPECT_FALSE(\1)", line)
    for op in ["EQ", "NE", "GT", "LT", "GE", "LE"]:
        line = re.sub(
            rf"REQUIRE_{op}\(\s*([^,]+),\s*(.+?)\s*\)", f"EXPECT_{op}(\\1, \\2)", line
        )

    def repl(m):
        inner = m.group(1).strip()
        eq = re.match(r"^(.+?)\s*==\s*(.+)$", inner)
        if eq:
            return f"EXPECT_EQ({eq.group(1).strip()}, {eq.group(2).strip()})"
        ne = re.match(r"^(.+?)\s*!=\s*(.+)$", inner)
        if ne:
            return f"EXPECT_NE({ne.group(1).strip()}, {ne.group(2).strip()})"
        return f"EXPECT_TRUE({inner})"

    return re.sub(r"REQUIRE\(\s*(.+?)\s*\)", repl, line)


def render(lines):
    """Convert REQUIRE macros and drop comments-only noise; returns code lines."""
    out = []
    for l in lines:
        s = l.strip()
        if s.startswith("//"):
            continue
        out.append(convert_require(l))
    return dedent(strip_blank(out))


def process(content, suite, header):
    lines = content.split("\n")

    # find SCENARIO blocks
    scenarios = []  # (name, body_lines)
    i = 0
    while i < len(lines):
        s = lines[i].strip()
        if s.startswith("SCENARIO("):
            text = lines[i]
            if s.startswith('SCENARIO(R"'):
                # multi-line raw string; ends at a line like ')" {'
                i += 1
                while not lines[i].strip().startswith(')"'):
                    text += "\n" + lines[i]
                    i += 1
                text += "\n" + lines[i]
            name = extract_test_name(text)
            body_start = i + 1
            # find matching close of scenario body
            depth = 1
            j = body_start
            body = []
            while j < len(lines):
                st = lines[j].strip()
                if st == "}":
                    depth -= 1
                    if depth == 0:
                        break
                    body.append(lines[j])
                else:
                    body.append(lines[j])
                    depth += st.count("{") - st.count("}")
                j += 1
            scenarios.append((name, body))
            i = j + 1
            continue
        i += 1

    tests = []  # (test_name, code_lines)
    used = set()

    def add(base_parts, code):
        base = "__".join(sanitize(p) for p in base_parts if p)
        name, n = base, 2
        while name in used:
            name = f"{base}_{n}"
            n += 1
        used.add(name)
        tests.append((name, code))

    for scen_name, body in scenarios:
        top = parse_items(body)
        global_setup = []
        for kind, *rest in top:
            if kind == "stmt":
                global_setup += rest[0]
        global_code = render(global_setup)

        for kind, *rest in top:
            if kind != "block":
                continue
            _, given_desc, given_inner = rest
            gitems = parse_items(given_inner)
            given_setup = []
            whens = []
            for k2, *r2 in gitems:
                if k2 == "stmt":
                    given_setup += r2[0]
                else:
                    _, wdesc, winner = r2
                    whens.append((wdesc, winner))
            given_code = render(given_setup)

            if not whens:
                add([scen_name, given_desc], global_code + given_code)
            for wdesc, winner in whens:
                wcode = render(flatten_then(winner))
                add([scen_name, given_desc, wdesc], global_code + given_code + wcode)

    out = [
        "// Test cases for Orderly Chaos",
        "//",
        "// Copyright (c) 2020 Christian Kauten",
        "//",
        "// Converted from Catch2 (SCENARIO/GIVEN/WHEN/THEN) to Google Test.",
        "// One TEST is emitted per Catch2 leaf section so that each WHEN",
        "// re-runs its GIVEN setup, matching Catch2 section semantics.",
        "",
        "#include <gtest/gtest.h>",
        f'#include "{header}"',
        "",
        "using namespace LOB;",
        "",
    ]
    for name, code in tests:
        out.append(f"TEST({suite}, {name}) {{")
        out += ["    " + c for c in code]
        out.append("}")
        out.append("")
    out += [
        "int main(int argc, char **argv) {",
        "    testing::InitGoogleTest(&argc, argv);",
        "    return RUN_ALL_TESTS();",
        "}",
        "",
    ]
    result = "\n".join(out)
    result = re.sub(
        r"EXPECT_EQ\(\s*(&?\w[\w:.\->]*)\s*,\s*nullptr\s*\)",
        r"EXPECT_TRUE(\1 == nullptr)",
        result,
    )
    result = re.sub(
        r"EXPECT_EQ\(\s*nullptr\s*,\s*(&?\w[\w:.\->]*)\s*\)",
        r"EXPECT_TRUE(nullptr == \1)",
        result,
    )
    return result, len(tests)


files = [
    ("test/test_limit_order_book.cpp", "LimitOrderBook", "limit_order_book.hpp"),
    ("test/test_limit_tree.cpp", "LimitTree", "limit_tree.hpp"),
]

for filepath, suite, header in files:
    path = f"D:\\quant\\{filepath}"
    with open(path, "r", encoding="utf-8") as f:
        content = f.read()
    result, count = process(content, suite, header)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(result)
    print(f"{filepath}: {count} tests")
