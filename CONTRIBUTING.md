# Contributing to Orderly Chaos

Thank you for helping improve Orderly Chaos. This guide explains how to set up
a development environment, the standards a change must meet, and how to
submit it.

## Ways to contribute

- **Report a bug:** open an issue with a minimal reproduction, the expected and
  actual behaviour, your operating system, compiler, and Python version.
- **Propose a feature:** open an issue describing the use case before writing
  code, so the design can be agreed first.
- **Improve the documentation:** corrections and clarifications are always
  welcome.
- **Report a vulnerability:** follow [SECURITY.md](SECURITY.md); do not open a
  public issue.

## Development setup

Requirements: a C++17 compiler, [Bazel](https://bazel.build/install) 7+ (or
Bazelisk), and Python 3.9+.

```bash
git clone https://github.com/Meetmendapara09/Orderly-Chaos.git
cd Orderly-Chaos
python -m venv .venv && source .venv/bin/activate
python -m pip install -e ".[test]"
make test
```

Run `make help` to list every target. Machine-specific Bazel options belong
in a git-ignored `user.bazelrc`, never in the shared `.bazelrc`.

## Making a change

1. Create a branch from `main`.
2. Make the change, keeping it focused on one concern.
3. Add or update tests (see below).
4. Update documentation: docstrings and header comments, the pages in `docs/`,
   the README if user-facing behaviour changes, and `CHANGELOG.md` under
   "Unreleased".
5. Run `make test` and make sure everything passes.
6. Open a pull request describing what changed and why.

## Standards

### Correctness and tests

- Every behaviour change needs a test. Add C++ tests to `tests/cpp/` and
  Python tests to `tests/python/`.
- Changes to matching logic must keep `tests/cpp/test_randomized.cpp` passing
  and, if the rules change, update its reference model to match.
- Call `check_invariants()` in new C++ tests that modify a book.
- Examples are executed by the test suite; keep them working.

### C++

- C++17, header-only for the core (`include/orderly_chaos/`).
- Four-space indentation, `snake_case` functions and variables, `PascalCase`
  types, and Doxygen (`///`) comments on every public declaration.
- Validate input before changing state, so failed calls never modify the book.
- Code must compile without warnings under
  `-Wall -Wextra -Wpedantic -Wshadow -Wconversion`.

### C API

- The C API is a stable ABI. Never change the meaning of an existing function,
  status value, or struct field; add new functions instead.
- No exception may escape an `extern "C"` function.
- Every new function needs a test in `tests/cpp/test_c_api.cpp` and an entry in
  `python/orderly_chaos/_library.py` if Python exposes it.

### Python

- Support Python 3.9+, use type hints, and write Google-style docstrings.
- Validate integer ranges before calling into C (ctypes silently truncates).
- Raise the typed exceptions in `orderly_chaos.errors`.

### Documentation

- Write in plain language with short sentences. Do not use em or en dashes.
- Diagrams use Mermaid; see [docs/README.md](docs/README.md).

## Versioning and releases

The project follows [Semantic Versioning](https://semver.org/). The version
appears in `python/orderly_chaos/_version.py` (canonical),
`include/orderly_chaos/version.hpp`, `pyproject.toml`, `MODULE.bazel`, and
the docs header; `python tools/check_versions.py` (and the CI docs job) fail
if they disagree. To release:

1. Update the version in all the files above (or run the check to find them).
2. Move the "Unreleased" notes in `CHANGELOG.md` under the new version.
3. Run `make test`, `make docker-test`, and `make dist`, then tag the commit `vX.Y.Z` and push the tag.
4. Pushing the tag builds and pushes the Docker image to GHCR as
   `ghcr.io/<owner>/orderly-chaos:vX.Y.Z` (plus `latest` for the default branch).
5. Publish a GitHub Release for the tag. The publish workflow builds the
   sdist and wheel, smoke-tests the sdist, and uploads both to PyPI, making
   `pip install orderly-chaos==X.Y.Z` available.

### One-time publishing setup (package owner only)

- **PyPI:** this repository uses trusted publishing, so no API token is
  stored anywhere. Before the first release, add a pending publisher at
  <https://pypi.org/manage/account/publishing> for this repository with
  workflow file name `publish.yml`. No other configuration is needed.
  If publishing fails with `invalid-publisher ... no corresponding
  publisher`, the pending publisher is missing or names the wrong
  repository, workflow file, or PyPI project (`orderly-chaos`); fix it on
  PyPI and re-run the workflow. As a fallback you may instead create a
  PyPI API token, store it as a `PYPI_API_TOKEN` repository secret, and
  re-run: the publish workflow uses it automatically when present.
- **GHCR:** pushing images uses the built-in `GITHUB_TOKEN`; no setup needed.
  If the package page does not appear under your profile, check the
  repository Settings, Actions, General, Workflow permissions
  ("Read and write permissions" for packages).

## License

By contributing you agree that your contributions are licensed under the
[MIT License](LICENSE).
