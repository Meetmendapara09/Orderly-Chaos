# Security policy

## Supported versions

| Version | Supported |
|---------|-----------|
| 0.2.x | Yes |
| 0.1.x | No; please upgrade (0.1 accepted inputs that could corrupt memory) |

## Reporting a vulnerability

Please do **not** open a public issue for security problems. Instead, report
them privately through
[GitHub Security Advisories](https://github.com/Meetmendapara09/Orderly-Chaos/security/advisories/new)
or by email to meetmendapara09@gmail.com.

Include a description of the issue, steps or code to reproduce it, the
affected version, and its impact. You can expect an acknowledgement within
five working days and a status update within fourteen days. Once a fix is
available it will be released and credited to you unless you prefer
otherwise.

## Scope and threat model

Orderly Chaos is an in-process library. It validates every input it receives,
never lets an exception cross the C boundary, and keeps the book consistent
when a request is rejected. It does not authenticate callers, enforce risk
limits, or persist data; applications that accept orders from untrusted
sources must provide those controls themselves.

Issues considered security vulnerabilities include memory corruption, crashes,
or unbounded resource use reachable through the public C, C++, or Python API
with any input values.
