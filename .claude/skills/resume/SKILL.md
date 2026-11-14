---
name: resume
description: Start a fresh mbingjustread-md session from where the last one stopped. Reads docs/handoff.md and the docs the next step needs, checks that the repo, build, tests and installed package are as described, reports a short status, then continues with the first next step. Use at the start of a new session, after /clear, or when the user says "resume", "continue", "where were we", "lanjut".
---

# Resume work

## 1. Read (in this order)

1. `docs/handoff.md`: the source of truth for state and next steps. If it is
   missing, say so and rebuild the picture from `git log --oneline -20`,
   `docs/ROADMAP.md` and `docs/PLAN.md`.
2. `docs/PLAN.md` (the one job, the decisions, known limits) and the current
   phase of `docs/ROADMAP.md`.
3. Only what the next step needs: `docs/ARCHITECTURE.md` before touching
   `src/`, `docs/MARKDOWN-SUPPORT.md` before touching the renderer or the
   highlighter.
4. Before writing or changing app code, load the `omarchy-app` skill: its
   conventions (flat `src/`, theme, translations, keyboard first, offscreen
   harness) are this project's conventions.

The auto-memory is already loaded; follow it.

## 2. Check the handoff is still true (quick, in parallel)

```bash
git fetch -q && git status -sb && git log --oneline -5
bin/test 2>&1 | tail -3
pacman -Q mbingjustread-md
```

If anything differs from the handoff (new commits, uncommitted files, failing
tests, a different installed version), say so before continuing. Don't fix it
silently.

## 3. Report, then continue

Give the user a short status (5–8 lines): where things stand, what is waiting
on them, what you found in step 2, and the step you are starting. Then start
the first next step that is doable now. Stop to ask only when the step is
outward-facing or hard to undo (a release or tag, a package sent to another
repository, rewriting pushed history), or the handoff says it waits on the
user.

## Working rules to keep

- The app only reads. No feature that edits or writes the opened file, ever.
- Don't reopen decisions in `docs/PLAN.md` unless the user asks.
- After changing the renderer, look at `tests/fixtures/showcase.md` in the
  app (an offscreen screenshot is enough), not just at the tests.
- Rendering stays linear in document size; `bin/test` has a case for it.
- Every visible string goes through `qsTr`/`tr` and gets an entry in
  `src/i18n/id.json`.
- `bin/install` needs sudo, so the user runs it (`! bin/install`).
- Commit in small, verified steps and push to `main`.
- Before ending the session, run `/wrap-up`.
