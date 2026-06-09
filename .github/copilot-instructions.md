# GitHub Copilot instructions for PowerTOP

## Essential context files

Always read these files at the start of any task:

- `local.md` — project-specific conventions, build commands, commit rules,
  and accumulated learnings from prior sessions. **Read this first.**
- `review/review.md` — code review process and checklist.
- `review/style.md` — coding style guide.
- `review/general-c++.md` — C++ coding rules.
- `review/rules.md` — PowerTOP-specific coding rules (divide-by-zero guards,
  pointer hygiene, etc.).
- `review/class.md` — class hierarchy; keep this up to date when changing
  the class structure.
- `review/tools.md` — tools and scripts used in the project.
- `tests/testdesign.md` — test design rules; read when working on the test suite.

## Build system

PowerTOP uses **Meson + Ninja**. Do not use autoconf/make.

```bash
meson setup build          # configure (once)
ninja -C build             # compile

meson setup -Denable-tests=true build_tf
ninja -C build_tf test     # build + run tests
```

For coverage instrumentation use the `build_cov` directory:
```bash
meson setup -Denable-tests=true -Db_coverage=true build_cov
ninja -C build_cov test
bash scripts/coverage_report.sh <label> build_cov
```

## C++ classes note

When creating a subclass, add or update the header of the parent class with a table that matches this example template:
```c
/*

# subclasses of <parent class>

| subclass     | filename                                             |
| ------------ | ---------------------------------------------------- | 
| <subclass 1> | <project relative path to the header for subclass 1> |

*/
```
Check is such comment with table already exists, and add a line when it does.
When no such comment exists, create a new comment ABOVE the class definition.

## Code review process

Follow `review/review.md` exactly:
1. Gather full function context (not just the changed lines).
2. Load `style.md`, `general-c++.md`, and `rules.md` into context.
3. Report findings with file + line references.

## Commit conventions

- Use `--no-gpg-sign` on all commits (GPG agent is not available).
- Write commit messages to a file in the project root, commit with `-F`, then
  delete the file. Do not use `/tmp`.
- Always include the co-author trailer:
  `Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>`
- Check for untracked temp files before `git add -A`.

## Testability pattern

When adding hardware I/O, extract calls into small `public virtual` helper
methods so unit tests can subclass and override them without real hardware.
See `tests/testdesign.md` and the existing bluetooth/ethernet/network tests
for examples.