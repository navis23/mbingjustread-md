# Architecture

```text
file.md
   │  read (never written)
   ▼
Backend ──► renderMarkdown() ──► HTML subset ──► TextEdit (Qt rich text)
   ▲             │                                   │
   │          md4c parser                            │ laid-out QTextDocument
   │          highlightCode()                        ▼
   └────────── outline, anchors, search ◄──── Backend::indexDocument()
```

## Files

| File | Job |
| --- | --- |
| `src/main.cpp` | Starts the app, creates the objects below, hands them to QML |
| `src/markdown.{h,cpp}` | `renderMarkdown()`: md4c callbacks to HTML. All decisions about how an element looks live here |
| `src/highlighter.{h,cpp}` | `highlightCode()`: a small lexer per language family |
| `src/backend.{h,cpp}` | The open file, history, zoom, live reload, outline, anchors, search, link handling |
| `src/theme.{h,cpp}` | The palette, from the Omarchy theme, with built-in dark and light fallbacks |
| `src/translator.{h,cpp}` | Follows the Omarchy language; catalogs are `src/i18n/<code>.json` |
| `src/filepicker.h` | Interface for "ask the user for a file" |
| `src/portalfilepicker.{h,cpp}` | The Linux implementation, through xdg-desktop-portal |
| `src/Main.qml` | The window: page, outline, search bar, help, shortcuts |

C++ owns everything that can fail. QML owns layout and keys.

## How a page is drawn

1. `Backend::load()` reads the file as UTF-8.
2. `renderMarkdown()` runs md4c over it. md4c calls back for each block,
   span, and piece of text; the `Renderer` class writes HTML for each.
3. QML sets that HTML on a read-only `TextEdit`, which lays it out.
4. QML hands the laid-out document back (`Backend::attachDocument()`), and the
   backend walks it to build the outline and the anchor table. Reading these
   from the real document, rather than from the Markdown, means a position
   in the outline is exactly a position on the page.

## Working within Qt's rich text

Qt lays out a subset of HTML 4 and CSS 2. Three of its limits shaped the
renderer, and each has a comment where it is handled:

- **No margins on tables.** Space after a code block, table, or quote is an
  explicit spacer paragraph (`Renderer::gap`).
- **No `border-left`, and only a table's own background fills reliably.** A
  quote's bar is an outer table in the bar color showing 4px beside an inner
  table in the page color (`Renderer::barBox`).
- **Images draw at natural size.** Local images are measured and given a
  width that fits the page (`Renderer::imageSize`); the page re-renders when
  its width changes.

Rendering must stay linear in document size. `bin/test` has a case that
renders a 600 KB document and fails if it gets slow.

## What is shared with the later ports

- macOS and Windows build the same sources. They need a `FilePicker` for the
  native dialog, the OS color scheme as the theme fallback, md4c vendored in
  (it is one C file), and packaging. Nothing in the renderer or backend is
  Linux-specific.
- The web version shares md4c (compiled to WebAssembly), the dialect flags,
  the heading-id rules (`Backend::slug`), and `tests/fixtures/showcase.md`
  as the page both must render the same.
