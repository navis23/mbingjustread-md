# Plan

## The one job

Read a Markdown file. Nothing else: no editing, no notes database, no export.
If a feature does not make reading a file better, it does not go in.

## What "good" means here

1. **Accurate.** Every structure in the file shows as its author meant it.
   Accuracy comes from the parser, so the parser is not ours: md4c implements
   the CommonMark spec and passes its test suite. We only decide how each
   element looks.
2. **Fast.** Opening a file should feel like opening an image. That rules out
   embedding a browser engine (hundreds of MB of memory, a second to start).
3. **Light.** One small binary, dependencies the system already has.
4. **Read-only and safe.** The file is opened for reading only. Raw HTML in a
   document is laid out as text formatting; nothing in a document can run.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Toolkit | C++ and Qt Quick, built with qmake | How Omarchy's own apps are built; also runs on macOS and Windows, so the later ports reuse the code |
| Parser | md4c (system library) | Spec-exact CommonMark and GFM, tiny, and already installed as a dependency of Qt |
| Rendering | md4c events → HTML subset → Qt rich text (`TextEdit`, read-only) | Native text layout with selection and copy across the whole page, no web engine |
| Code highlighting | Our own small lexer | Keeps the binary small; comments, strings, numbers, keywords, and calls are what the eye needs |
| File dialog | xdg-desktop-portal over D-Bus, behind a `FilePicker` interface | Native dialog on Linux; the interface is the seam for macOS and Windows |
| Packaging | Arch package via `makepkg` | Installs like any other Omarchy app and lands in the launcher |
| License | MIT in `LICENSE` (plain text), credits in `NOTICE` | Plain text so GitHub detects MIT; the Omacut and Monologue notice, which their MIT license requires, ships beside it in the package |

## Version 0.1 (this version): Linux and Omarchy

Done:

- Rendering of CommonMark, GFM tables, task lists, strikethrough, autolinks,
  footnotes, alerts, front matter, wiki links, highlights, images
- Code highlighting
- Outline, find, zoom, keyboard navigation, text selection and copy
- Links: in-page anchors, other Markdown files (with history), the web
- Live reload when the file changes
- Follows the Omarchy theme, rounding, and language (English, Indonesian)
- Arch package, launcher entry, `text/markdown` association
- Tests for the renderer, the backend, the theme, and the translator

Known limits, all on the roadmap:

- Math is shown as LaTeX source, not typeset
- Mermaid and other diagram blocks are shown as code
- A code line longer than the page wraps instead of scrolling sideways
- Images from the web load, but only local images are scaled to the page
- A very large file (hundreds of KB with thousands of tables or code blocks)
  takes a few seconds to open; scrolling it is instant

## What comes next

See [ROADMAP.md](ROADMAP.md). In short: polish on Linux, then the web
version, then macOS, then Windows.
