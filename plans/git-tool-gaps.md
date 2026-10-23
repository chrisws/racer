# Git Tooling — Known Gaps & Missing Features

_Dated: 2025-07-11_

## Issues Encountered

### 1. `TOOL:RUN` does not support shell operators
- `&&`, `||`, `;`, `|`, `>` are **not** interpreted as shell metacharacters.
- The entire argument string is passed as a single command invocation.
- **Workaround:** Issue separate `TOOL:RUN` calls for each command.
- **Example that fails:**
  ```
  TOOL:RUN git add src/car.c && git commit -m "msg"
  ```
- **Example that works:**
  ```
  TOOL:RUN git add src/car.c
  TOOL:RUN git commit -m "msg"
  ```

### 2. `TOOL:GIT commit` — "unsafe characters" in message
- Commit messages with certain characters (parentheses, quotes, etc.) can trigger
  an "args contain unsafe characters" error.
- **Workaround:** Use `TOOL:RUN git commit -m "message"` instead of `TOOL:GIT commit`.
- Keep messages simple: avoid parentheses, backticks, and special punctuation.

### 3. No `git rm --cached` in `TOOL:GIT`
- `TOOL:GIT` supports: status, log, diff, add, commit, branch, show.
- `git rm` (including `--cached`) is **not** available as a subcommand.
- **Workaround:** Use `TOOL:RUN git rm --cached -f <file>`.

### 4. No `git config` in `TOOL:GIT`
- Cannot set user.name / user.email via the tool.
- **Workaround:** Use `TOOL:RUN git config user.name "Name"`.

### 5. No `git submodule` support
- Cannot add, init, or update submodules via `TOOL:GIT`.
- **Workaround:** Use `TOOL:RUN git submodule add <url> <path>`.

### 6. No `git stash` support
- Cannot stash/unstash changes.
- **Workaround:** Use `TOOL:RUN git stash` / `TOOL:RUN git stash pop`.

### 7. No `git rebase` / `git merge` / `git cherry-pick`
- Only basic operations are exposed.
- **Workaround:** Use `TOOL:RUN` for these.

### 8. No `git tag` support
- Cannot create or list tags.
- **Workaround:** Use `TOOL:RUN git tag <name>`.

### 9. No `git push` / `git pull` / `git fetch`
- Remote operations are not available.
- **Workaround:** Use `TOOL:RUN git push origin master`, etc.

### 10. `TOOL:RUN` has no working-directory flag
- Cannot specify a subdirectory to run in (e.g., `cd build && make`).
- **Workaround:** Use absolute paths or run from sandbox root.

## Suggested Additions to `TOOL:GIT`

| Subcommand | Purpose |
|---|---|
| `rm` | Remove files (with `--cached` flag) |
| `config` | Get/set git config values |
| `submodule` | Add/init/update submodules |
| `stash` | Stash/pop/drop |
| `tag` | Create/list/delete tags |
| `push` / `pull` / `fetch` | Remote operations |
| `rebase` / `merge` | History manipulation |

## Suggested `TOOL:RUN` Improvements

- Support for `&&` and `;` chaining (or a `--shell` flag)
- Working directory flag: `TOOL:RUN --cwd=build make`
- Environment variable passthrough: `TOOL:RUN --env=CC=clang make`

## Current Workaround Pattern

For multi-step git operations, always issue **one command per `TOOL:RUN` call**:

```
TOOL:RUN git add src/car.c
TOOL:RUN git commit -m "Fix friction"
```

Never chain with `&&` or `;`.