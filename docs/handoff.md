# Handoff — 2026-11-16

> Written by `/wrap-up` for the next session. Start with `/resume`.

## Snapshot
- Branch: `main`, pushed to `origin` (https://github.com/navis23/mbingjustread-md,
  **public** since 2026-11-16). The newest commit is this handoff; before it, `7e4e52b`.
- Version: `0.1.0` in the repo · installed on this machine: `mbingjustread-md 0.1.0-1`
  (built before this session's outline fix; `bin/install` again to get it)
- Tests: 26 passed, 0 failed (`bin/test`, run 2026-11-16)
- Roadmap phase: 0.2 — Linux polish (see [ROADMAP.md](ROADMAP.md))
- Processes left running: none

## What exists now
- A read-only Markdown reader for Omarchy: C++ and Qt Quick, about 400 KB,
  md4c as the parser, no browser engine. Goal and decisions: [PLAN.md](PLAN.md).
- Renders CommonMark, the GitHub extensions, front matter, wiki links,
  highlights, images, highlighted code: [MARKDOWN-SUPPORT.md](MARKDOWN-SUPPORT.md).
- Outline, find, zoom, vim-style keys, links between `.md` files with back
  and forward, live reload. Shortcuts are in the README and under `?`.
- Follows the Omarchy theme, rounding, and language (English, Indonesian).
- Arch package (`bin/install`), launcher entry, `text/markdown` association.
- Open source: MIT in `LICENSE`, the Omacut/Monologue credit in `NOTICE`
  (both installed under `/usr/share/licenses/mbingjustread-md/`).
- How the code fits together: [ARCHITECTURE.md](ARCHITECTURE.md).

## This session
- History rewritten (all commits, before going public): the author email is
  now the GitHub noreply address, and this repo's `git config user.email` is
  set to it. All hashes changed; the old v0.1 commit is now `09657d5`.
- `957664e` The outline marks the section being read: the last heading above
  a line a quarter of the way down the view (`updateCurrentHeading` in
  `src/Main.qml`), and a jump (outline click, `#anchor` link) marks the
  heading it went to (`showPosition`).
- `7e4e52b` Plain MIT `LICENSE` so GitHub detects it; credits moved to
  `NOTICE`, installed by the PKGBUILD; credit comments in `src/translator.cpp`
  and `src/portalfilepicker.cpp`.
- Repo made public, with a description and topics.

## Verified, and not
- Verified: 26 tests; the outline fix in six offscreen screenshots (heading
  just below the top, scrolled from the top, at the top, jump to the last
  heading, End); the package contains `LICENSE` and `NOTICE` (built with
  `makepkg`, not installed); GitHub shows the license as MIT.
- Not verified (carried over): the file-open dialog (`O`, through the
  portal), drag and drop, mouse selection, `Ctrl+C`, opening a `.md` from the
  file manager, images loaded from the web.
- Seen once, cause unknown (carried over): one code line looked bold in a
  search screenshot, probably the software renderer `bin/shot` uses.
- Known and accepted: after `End`, the outline marks the section at the top
  of the view, so the last short sections are only marked by clicking them.

## In progress / unfinished
Nothing half-done.

## Next steps (in order)
1. Ask the user how the installed app feels in daily use, and what bothered
   them. Their answer orders the rest of 0.2. — waiting on the user
2. Try the unverified items above in the real window and fix what breaks;
   the portal dialog is the most likely to need work. — needs the user to click
3. Go through 0.2 in [ROADMAP.md](ROADMAP.md). Suggested order: faster
   opening of very large files, sideways scrolling for wide code and tables,
   remembered scroll position, scaling of web images, math. — doable now
4. Release: tag `v0.1.0` (or `v0.2.0` after polish), GitHub release, then the
   package for `omacom/omarchy-pkgs` and the AUR. — needs the user's go-ahead
5. 0.3 — the web version in `web/`, as planned in the roadmap. — after 0.2

## Waiting on the user
- Feedback after using the app (step 1).
- `! bin/install` to get the outline fix into the installed app.
- Whether to make it the default for Markdown:
  `xdg-mime default mbingjustread-md.desktop text/markdown`.
- Go-ahead before any release, tag, or package sent elsewhere.
- A local branch `backup-before-email-rewrite` holds the old history (never
  pushed). The user can delete it: `git branch -D backup-before-email-rewrite`.
  Never push it.

## Gotchas
- **The repo is public.** Commits must use the noreply email (already set in
  this repo's git config); keep personal data out of fixtures, screenshots,
  and docs.
- **Qt rich text is not a browser.** Tables take no margins (spacer paragraph,
  `Renderer::gap`); only a table's own `bgcolor` fills its box; `<pre>` leaves
  a blank line; an empty spacer paragraph is swallowed by the table before it
  (use `&nbsp;`). Read "Working within Qt's rich text" in ARCHITECTURE.md
  before changing `src/markdown.cpp`.
- **Keep rendering linear.** There is a test for it.
- **Large files:** 630 KB with 2,800 tables opens in about 3 s, nearly all Qt
  layout; toggling the outline reflows it. First 0.2 item.
- **Stopping the app from a shell:** `pgrep -x mbingjustread-m | xargs -r kill`.
  `pkill -f mbingjustread` kills the shell that runs it.
- `bin/shot` with an empty directory as the theme gives the built-in palette
  and English; `magick a.png b.png +append out.png` puts screenshots side by
  side (no Python PIL on this machine).
- QML warnings only reach the terminal with `QT_FORCE_STDERR_LOGGING=1`.
- `bin/install` asks for sudo, so the user runs it: `! bin/install`.
- At start the app logs "Failed to register with host portal …". Harmless so
  far; look at it when testing the open dialog.
- A pending `#anchor` for a link to another file must be set before
  `Backend::open()`, because indexing happens inside it.

## Resume commands
```bash
bin/build && build/mbingjustread-md tests/fixtures/showcase.md   # run it
bin/test                                                         # 26 tests
bin/shot - tests/fixtures/showcase.md /tmp/page.png T find:table # screenshot
bin/install                                                      # user runs this (sudo)
```
