# Third-party notices

Orderly Chaos is created and maintained by Meet Mendapara and released under
the [MIT License](LICENSE). It includes or builds on the following third-party
work. Each component remains under its own license, reproduced or referenced
below.

## Bundled in this repository

### tsl::robin_map 0.6.1

- **Location:** [`third_party/robin_map/`](third_party/robin_map/)
- **Used for:** the hash index from price to price level
- **Source:** <https://github.com/Tessil/robin-map>
- **License:** MIT, Copyright (c) 2017 Tessil.
  Full text: [`third_party/robin_map/LICENSE`](third_party/robin_map/LICENSE)

## Derived work

### limit-order-book

- **Source:** <https://github.com/Kautenja/limit-order-book>
- **License:** MIT, Copyright (c) 2019-2020 Christian Kauten
- **Relationship:** Orderly Chaos began as a fork of this project. The engine
  has since been substantially rewritten (balanced price-level tree, input
  validation, trade events, C API, Python package, and tests), and the
  original copyright notice is retained in [`LICENSE`](LICENSE) as the MIT
  License requires.

## Fetched at build time (not redistributed)

### GoogleTest 1.14.0

- **Used for:** C++ unit tests only; not part of any shipped library
- **Source:** <https://github.com/google/googletest>, pinned by SHA-256 in
  [`MODULE.bazel`](MODULE.bazel)
- **License:** BSD 3-Clause, Copyright 2008 Google Inc.

## Used by the documentation website (loaded from a CDN)

| Library | Version | License |
|---------|---------|---------|
| [Mermaid](https://mermaid.js.org/) | 12.1.0 | MIT |
| [highlight.js](https://highlightjs.org/) | 11.12.0 | BSD 3-Clause |
| [IBM Plex Sans](https://github.com/IBM/plex) and [Source Code Pro](https://github.com/adobe-fonts/source-code-pro) (Google Fonts) | latest | SIL Open Font License 1.1 |
