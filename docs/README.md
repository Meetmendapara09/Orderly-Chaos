# Documentation

This directory contains the Orderly Chaos documentation website (plain static
HTML, no build step) and Markdown design notes.

## Pages

| Page | Audience | Contents |
|------|----------|----------|
| [index.html](index.html) | Everyone | Landing page |
| [overview.html](overview.html) | New users | What it is, use cases, when to use it, matching rules, lifecycle, errors |
| [getting-started.html](getting-started.html) | New users | Installation for Python, C++, and C; verification; first program; troubleshooting |
| [architecture.html](architecture.html) and [architecture.md](architecture.md) | Contributors, evaluators | Layers, data structures, algorithm, complexity, guarantees |
| [api/python.html](api/python.html) | Python users | Complete Python reference |
| [api/cpp.html](api/cpp.html) | C++ users | Complete C++ reference |
| [api/c.html](api/c.html) | C and FFI users | Complete C reference |
| [examples/index.html](examples/index.html) | Everyone | Walkthroughs of the programs in `examples/` |
| [benchmarks.html](benchmarks.html) | Evaluators | Results, methodology, reproduction |

## Preview locally

```bash
make serve-docs          # http://localhost:8000
# or
python -m http.server --directory docs 8000
```

Use a local server rather than opening files directly: the shared header and
footer are loaded with `fetch`, which browsers block for `file://` URLs (a
minimal fallback navigation is shown in that case).

## Structure

```text
docs/
  *.html, api/, examples/    Pages
  components/                Shared header and footer fragments
  assets/css/styles.css      The single stylesheet (design tokens at the top)
  assets/js/components.js    Injects the header and footer, marks the active link
  assets/js/site.js          Syntax highlighting, Mermaid diagrams, mobile menu, TOC scrollspy
  assets/js/copy-code.js     Copy buttons on code blocks
  assets/js/hero.js          Landing page order book animation
  assets/favicon.svg        Depth-ladder mark (source of truth for icons)
  assets/favicon-16x16.png, assets/favicon-32x32.png
                            Raster favicons generated from favicon.svg
  assets/apple-touch-icon.png
                            180px icon for iOS home screen
  assets/site.webmanifest    Web app manifest (icons, theme color)
  architecture.md            Design notes (rendered by GitHub)
```

## Libraries

| Library | Purpose | How it is loaded |
|---------|---------|------------------|
| [Mermaid 12.1.0](https://mermaid.js.org/) | Flowcharts, sequence, class, and state diagrams | ES module from jsDelivr, imported only on pages with diagrams |
| [highlight.js 11.12.0](https://highlightjs.org/) | Syntax highlighting | jsDelivr with a Subresource Integrity hash |
| IBM Plex Sans, Source Code Pro | Typography | Google Fonts |

Versions are pinned so that the site renders the same way over time.

## Writing documentation

- Every page uses the same skeleton: copy an existing page in the same
  directory and keep the `<head>` scripts and the header and footer slots.
- Set `<body data-page="...">` to the matching `data-page` value in
  `components/header.html` so the active link is highlighted.
- Wrap tables in `<div class="table-wrap">` so they scroll on small screens.
- Mark code blocks with a language: `<pre><code class="language-python">`.
  Escape `<`, `>`, and `&` inside code.
- Diagrams go in `<figure class="diagram"><pre class="mermaid">...</pre></figure>`.
  Do not hard-code colours in diagrams; the theme in `site.js` styles them.
- Keep examples runnable. Prefer copying from `examples/`, which the test
  suite executes.
- Write in plain language: short sentences, no em or en dashes.
