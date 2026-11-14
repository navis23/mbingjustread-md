# Handoff — 2026-11-14

> Written by `/wrap-up` for the next session. Start with `/resume`.

## Snapshot
- Branch: `main`, pushed to `origin` (https://github.com/navis23/mbingjustread-md).
  Check with `git log --oneline -3`; the newest commit is this handoff.
- Version: `0.1.0` in the repo · installed on this machine: `mbingjustread-md 0.1.0-1`
- Tests: 26 passed, 0 failed (`bin/test`, run 2026-11-14)
- Roadmap phase: 0.1 done; next is 0.2 — Linux polish (see [ROADMAP.md](ROADMAP.md))
- Processes left running: none

## What exists now
- A read-only Markdown reader for Omarchy: C++ and Qt Quick, about 400 KB,
  md4c as the parser, no browser engine. Goal and decisions: [PLAN.md](PLAN.md).
- Renders CommonMark and the GitHub extensions, front matter, wiki links,
  highlights, images, and highlighted code:
  [MARKDOWN-SUPPORT.md](MARKDOWN-SUPPORT.md).
- Outline, find, zoom, vim-style keys, links between `.md` files with back
  and forward, live reload. Shortcuts are in the README and under `?`.
- Follows the Omarchy theme, rounding, and language (English, Indonesian).
- Arch package (`bin/install`), launcher entry, `text/markdown` association.
- How the code fits together: [ARCHITECTURE.md](ARCHITECTURE.md).
- Project skills: `/wrap-up` and `/resume` in `.claude/skills/`.

## This session
- `fc1ef8d` Read Markdown files: the whole of v0.1, with docs and tests.
- `bin/shot` and `tests/shot/`: the offscreen screenshot tool, moved into
  the repo so it survives between sessions.
- `.claude/skills/wrap-up`, `.claude/skills/resume`, and this file.

## Verified, and not
- Verified: 26 tests; the package builds and is installed; the real binary
  was opened once on the desktop and rendered in the user's theme; outline,
  search, help overlay (also in Indonesian), keyboard scrolling, links, and
  the fallback palette were checked in offscreen screenshots.
- Not verified: the file-open dialog (`O`, through the portal), drag and
  drop, selecting text with the mouse, `Ctrl+C`, opening a `.md` from the
  file manager, and images loaded from the web.
- Seen once, cause unknown: one code line looked bold in a search
  screenshot. Suspected to come from the software renderer `bin/shot` uses;
  not checked in the real window.
- Seen in the last screenshot: with the outline open and the "Table" heading
  near the top of the page, the outline marked "Code" (the section before).
  Check `updateCurrentHeading` in `src/Main.qml`.

## In progress / unfinished
Nothing half-done.

## Next steps (in order)
1. Ask the user how the installed app feels in daily use, and what bothered
   them. Their answer should order the rest of 0.2. — waiting on the user
2. Try the unverified items above in the real window and fix what breaks.
   The portal dialog is the most likely to need work. — doable now, though
   the dialog needs the user to click
3. Fix the outline marking the previous section (see above). — doable now
4. Go through 0.2 in [ROADMAP.md](ROADMAP.md). Suggested order: faster
   opening of very large files, sideways scrolling for wide code and
   tables, remembered scroll position, scaling of web images, math. —
   doable now
5. Release: tag `v0.1.0` (or `v0.2.0` after polish), GitHub release, then
   the package for `omacom/omarchy-pkgs` and the AUR. — needs the user's
   go-ahead; it is public and goes to other repositories
6. 0.3 — the web version in `web/`, as planned in the roadmap. — after 0.2

## Waiting on the user
- Feedback after using the app (step 1).
- Whether to make it the default for Markdown:
  `xdg-mime default mbingjustread-md.desktop text/markdown`.
- Go-ahead before any release, tag, or package sent elsewhere.

## Gotchas
- **Qt rich text is not a browser.** Tables take no margins (spacing is a
  spacer paragraph, `Renderer::gap`); only a table's own `bgcolor` fills its
  box (code boxes and quote bars are tables); `<pre>` leaves a blank line
  (code uses `<td>` with `white-space:pre-wrap`); an empty spacer paragraph
  gets swallowed by the table before it (use `&nbsp;`). Read the "Working
  within Qt's rich text" part of ARCHITECTURE.md before changing
  `src/markdown.cpp`.
- **Keep rendering linear.** A regular expression run on the whole output
  per block made a 630 KB file take 20 s. It is now about 0.1 s and there
  is a test for it.
- **Large files:** 630 KB with 2,800 tables opens in about 3 s, nearly all
  of it Qt laying the document out; toggling the outline reflows it (about
  3.5 s). This is the first 0.2 item.
- **Memory** was about 170 MB for the running app, mostly Qt Quick.
- **Stopping the app from a shell:** `pgrep -x mbingjustread-m | xargs -r kill`
  (the process name is cut at 15 characters). `pkill -f mbingjustread`
  kills the shell that runs it.
- QML warnings only reach the terminal with `QT_FORCE_STDERR_LOGGING=1`
  (`bin/shot` sets it).
- `bin/install` asks for sudo, so the user runs it: `! bin/install`.
- At start the app logs "Failed to register with host portal … Connection
  already associated with an application ID". Harmless so far, but look at
  it when testing the open dialog.
- A pending `#anchor` for a link to another file must be set before
  `Backend::open()`, because indexing happens inside it.

## Resume commands
```bash
bin/build && build/mbingjustread-md tests/fixtures/showcase.md   # run it
bin/test                                                         # 26 tests
bin/shot - tests/fixtures/showcase.md /tmp/page.png T find:table # screenshot
bin/install                                                      # user runs this (sudo)
```
