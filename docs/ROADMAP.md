# Roadmap

Order: Linux polish → web → macOS → Windows. The web version goes before
macOS because it is the cheaper of the two and reaches every platform at
once (reasons below).

## 0.1 — Linux, Omarchy (done)

See [PLAN.md](PLAN.md).

## 0.2 — Linux polish

- Done 2026-11-16: the repository is public and open source (MIT, with the
  Omacut and Monologue credit in `NOTICE`)
- Done 2026-11-16: the outline marks the section being read, not the one
  before it
- Publish a GitHub release and add the package to `omacom/omarchy-pkgs`, so
  it installs with `omarchy-pkg-add`; an AUR package for other Arch users
- Faster opening of very large files (lay out the visible part first)
- Math typeset instead of shown as source
- Remember the scroll position per file, and restore it on back and forward
- Horizontal scrolling for wide code blocks and tables
- Scale images from the web to the page width
- Open several files (one window each is already possible; add a recent
  files list on the empty screen)
- More languages for the interface (catalogs are one JSON file each)
- AppImage or Flatpak for non-Arch distributions

## 0.3 — Web

**Is it easy? Yes.** The worry was the speed of reading files through the
browser. That is solved by the File System Access API: the browser hands the
page a direct handle to the local file, nothing is uploaded anywhere, and
reading a Markdown file takes a few milliseconds.

Plan:

- **A static site, no server.** All work happens in the browser. Host it
  anywhere that serves files (Vercel, GitHub Pages).
- **Same parser.** md4c compiled to WebAssembly (about 50 KB). The Markdown
  dialect is then identical to the desktop app by construction.
- **Its own renderer, HTML and CSS.** The desktop renderer writes HTML for
  Qt's text engine and works around its limits; a browser has none of those
  limits, so the web renderer is simpler and can do more (rounded code
  blocks, sideways scrolling, KaTeX for math, Mermaid for diagrams).
- **Opening files:**
  - `showOpenFilePicker()` for the Open button, and drag and drop
  - `showDirectoryPicker()` to open a folder once and browse its Markdown
    files in a sidebar; the handle is kept in IndexedDB, so the folder is
    still there next visit
  - Installed as a PWA with `file_handlers`, double-clicking a `.md` file in
    the file manager opens it in the app (Chrome and Edge)
  - Live reload by re-reading the handle when the tab regains focus
  - Firefox and Safari do not have the API yet; they fall back to a normal
    file input, which still reads locally and instantly, without the folder
    sidebar and live reload
- **Shared with the desktop app:** dialect flags, heading ids, keyboard
  shortcuts, and `tests/fixtures/showcase.md` as the conformance page.

Where it lives: a `web/` folder in this repository, so the fixture and the
docs stay in one place.

## 0.4 — macOS

Same C++ and QML sources. Work needed:

- `FilePicker` using the native open dialog
- Theme fallback from the system light or dark setting
- md4c vendored into the build
- Open files from Finder (`QFileOpenEvent`) and declare `.md` in `Info.plist`
- `Cmd` shortcuts beside the single-key ones
- `macdeployqt` into a `.dmg`; signing and notarization (needs an Apple
  developer account)
- A GitHub Actions job to build it

## 0.5 — Windows

- `FilePicker` using the native open dialog
- `windeployqt` and an installer (Inno Setup or MSIX), `.md` association
- A GitHub Actions job to build it

## Not planned

Editing, in any form. Use an editor; this reloads when you save.
