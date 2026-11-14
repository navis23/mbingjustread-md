---
name: wrap-up
description: End a mbingjustread-md work session cleanly before /clear. Checks the repo, the build, the tests and leftover processes, brings docs/ up to date, writes docs/handoff.md (state, what changed, next steps, open questions, gotchas, resume commands), commits and pushes it, then tells the user to run /clear and /resume. Use when the user says "wrap up", "handoff", "end session", "save progress", "we're switching session", or before a /clear.
---

# Wrap up the session

Goal: a fresh session with no memory of this conversation can continue from
`docs/handoff.md` alone, together with the rest of `docs/` and the
auto-memory. Write for that reader: concrete, current, no narrative.

## 1. Collect the facts (check, don't write from memory)

Run these, in parallel where possible:

```bash
date '+%Y-%m-%d %H:%M %Z'
git status -sb && git log --oneline -15
git diff --stat HEAD                       # uncommitted work
bin/test 2>&1 | tail -3                    # builds too; expect "Totals: N passed, 0 failed"
pacman -Q mbingjustread-md                 # installed version, if any
pgrep -x mbingjustread-m                   # app windows left open (name is cut at 15 chars)
```

- Stop app windows and harness processes **you** started this session:
  `pgrep -x mbingjustread-m | xargs -r kill`. Never `pkill -f mbingjustread`:
  it matches the shell running it. If the user has the reader open on their
  own file, leave it.
- Read the previous `docs/handoff.md`, `docs/ROADMAP.md` and the "Known
  limits" in `docs/PLAN.md`, so anything still open is carried forward.
- If the installed version is older than the repo version and the session
  changed what the user sees, note that `bin/install` must be run again (it
  needs sudo, so the user runs it: `! bin/install`).

## 2. Bring the docs up to date

- Roadmap items finished or discovered → `docs/ROADMAP.md` (change status and
  append; don't rewrite the plan).
- New decisions, with the reason → the Decisions table in `docs/PLAN.md`.
- A Markdown structure or language added or changed →
  `docs/MARKDOWN-SUPPORT.md`, and `tests/fixtures/showcase.md` has an example.
- A new source file or a changed data flow → `docs/ARCHITECTURE.md`.
- New shortcuts or install steps → `README.md` (and the help overlay in
  `src/Main.qml` and `src/i18n/id.json` must already match).

## 3. Write `docs/handoff.md`

Overwrite it using `handoff-template.md` (next to this file). Rules:

- Dates are absolute (YYYY-MM-DD). Commit hashes are short hashes that exist
  on the remote.
- "Next steps" is an ordered list. Step 1 is what the next session should
  actually do first, and why. Mark each as doable now or waiting on the user.
- Say plainly what was verified and how (tests, offscreen screenshot, real
  window), and what was not.
- Under ~120 lines; link to `docs/*` instead of repeating it.
- No secrets, and no personal file names or contents from the user's machine.

## 4. Commit and push

Commit unrelated uncommitted work first, as its own commit with a proper
message (short and imperative, like "Scroll wide code blocks sideways").
Then:

```bash
git add docs/ .claude/skills && git commit -m "Handoff: <one-line summary of the session>" && git push
```

End commit messages with the attribution lines from the system reminder, if
any. Push only to `main` on `origin`
(https://github.com/navis23/mbingjustread-md), which the user asked for.
Releases, tags, and anything sent to other repositories (`omarchy-pkgs`, AUR)
need the user's go-ahead first.

## 5. Save durable feedback

If the user corrected how the work should be done, save it to memory. Project
facts belong in `docs/`, not in memory.

## 6. Tell the user

Reply briefly:

- what the session delivered (3–5 bullets),
- anything that needs their action,
- then: "Run `/clear`, then `/resume` to continue from `docs/handoff.md`."
