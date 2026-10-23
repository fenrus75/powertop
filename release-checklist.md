# PowerTOP release checklist

Prior to making a release or release candidate, perform all of the
checks below to ensure a smooth experience for our users.

> **Agent note:** When performing these steps, treat every section as
> a gate — do not proceed to the next section if any item in the
> current section fails.

## Version bump

- [ ] Ask the user for the new version number.
      Suggest at least two options (use `git tag` to check the current latest):
      1. Next minor/major release (e.g. `v2.X`)
      2. Next release candidate (e.g. `v2.X-rc1`)
- [ ] Update the version in `meson.build` (`version:` field)
- [ ] Verify the version appears correctly in `powertop --version` output

## Translations

- [ ] Audit `POTFILES.in`: find all source files containing `_()` or `N_()` that
      are not yet listed, add them, then remove any entries for files that no
      longer exist.
      ```
      # Files with translatable strings not in POTFILES.in:
      comm -23 <(grep -rl '\b_(\|N_(' --include="*.cpp" --include="*.h" src/ | sort) \
               <(grep -v '^#\|^$' po/POTFILES.in | sort)
      ```
- [ ] Regenerate `po/powertop.pot` from all listed source files:
      ```
      xgettext --files-from=po/POTFILES.in --directory=. \
        --default-domain=powertop --output=po/powertop.pot \
        --language=C++ --keyword=_ --keyword=N_ --from-code=UTF-8 \
        --add-comments=TRANSLATORS --copyright-holder="PowerTOP contributors" \
        --package-name=powertop
      ```
- [ ] Merge updated template into all `.po` files (with fuzzy matching to
      preserve translations for mechanically-changed strings):
      ```
      while IFS= read -r lang; do
        [[ "$lang" =~ ^# ]] || [[ -z "$lang" ]] && continue
        msgmerge --update --backup=none po/${lang}.po po/powertop.pot
      done < po/LINGUAS
      ```

## Build checks (all must compile with zero errors and zero warnings)

Each build must be a **clean build** (`meson setup --wipe`), not incremental.
Warnings are an **absolute zero** requirement — pre-existing warnings must be
fixed before release, not excused.

Only ninja compiler output counts; `meson setup` warnings (e.g. netrc
permissions) are not compiler warnings and can be ignored.

- [ ] Standard build: `meson setup --wipe -Denable-tests=true build`
- [ ] Release build:  `meson setup --wipe --buildtype=release -Denable-tests=true build_release`
- [ ] Debug build:    `meson setup --wipe --buildtype=debug   -Denable-tests=true build_debug`
- [ ] ASAN build:     `meson setup --wipe -Denable-tests=true -Db_coverage=true -Db_sanitize=address build_acov`
- [ ] gcov build:     `meson setup --wipe -Denable-tests=true -Db_coverage=true build_cov`

Check each with: `ninja -C <dir>` and read the full output directly.
**Do not pipe ninja through grep** — grep returns exit code 1 when it finds
no matches, making a clean build falsely appear to fail.

## Tests (all five build types, all must pass with no failures)

- [ ] Standard build: `ninja -C build test`
- [ ] Release build:  `ninja -C build_release test`
- [ ] Debug build:    `ninja -C build_debug test`
- [ ] ASAN build:     `ninja -C build_acov test`
- [ ] gcov build:     `ninja -C build_cov test`

The ASAN build will have fewer total tests as the valgrind tests do not run
in that build; this is expected.

## Memory leak check

- [ ] Valgrind full leak check (use the uninstrumented standard build):
      ```
      sudo valgrind --leak-check=full --show-leak-kinds=all build/powertop --html --time=3
      ```
      Only the known library-internal leak is acceptable (see `local.md`):
      - 20 bytes: libtracefs internal strdup
      Any new leaks must be fixed before release.

- [ ] ASAN leak check (confirms the same with sanitizer instrumentation):
      ```
      ASAN_OPTIONS=detect_leaks=1 sudo -E build_acov/powertop --once --time=3 2>&1 | grep -i "leak\|SUMMARY"
      ```
      No leaks are expected from PowerTOP code.

## Test coverage

Coverage is collected from `build_acov` (ASAN + gcov), which gives the most
instrumentation. Run the test suite first, then the `--once` run to cover the
display/hardware paths that tests cannot reach.

- [ ] Run the test suite to populate coverage data:
      `ninja -C build_acov test`
- [ ] Run a live measurement to cover display and hardware paths:
      `ASAN_OPTIONS=detect_leaks=0 sudo -E build_acov/powertop --once --time=3`
- [ ] Capture the combined coverage snapshot:
      `bash scripts/coverage_report.sh <version-label> build_acov`
- [ ] Review the report for any significant regressions vs. the previous release.

## Release notes

Store in `doc/relnotes.md`. Use `git shortlog <prev-stable-tag>..` to enumerate commits,
where `<prev-stable-tag>` is the **last stable release** (e.g., `v2.15`), not the previous RC.
This ensures the notes are cumulative and cover all RC changes as well.
For the v2.16 cycle the base was `v2.15`.

**RC cadence**: the file has a single section for the whole release cycle. When moving from
`-rc1` to `-rc2` (or `-rc3`, etc.) **update the heading and add new items in-place** — do not
create a new section.  The final stable tag just renames the heading (e.g., `## v2.16-rc2` →
`## v2.16`).

- [ ] Update the version heading in `doc/relnotes.md` to match the new tag
      (for RCs: add new changes to the existing sections; do not duplicate)
- [ ] Update the "Recent releases" table near the top of `README.md`
      with the new version and a one-line summary
- [ ] Summary of top user-visible enhancements and changes
      (new tunables, UI changes, other new features)
- [ ] **New tunables**: users care about these most — enumerate explicitly.
      Find additions with:
      ```
      git diff <prev-stable-tag>.. -- src/tuning/tuning.cpp | grep "^+.*add_sysfs_tunable"
      ```
      List each new tunable with its description, sysfs path, and suggested value.
      Also check for wildcard-path tunables (paths containing `*`) which apply to
      multiple devices at once.
- [ ] Summary of new command-line options
- [ ] **Short** summary of internal changes (no more than 3 lines for all changes combined)
- [ ] **Thank external contributors**: run `git shortlog -sn <prev-stable-tag>..` and add a
      "Thanks" section listing anyone who is not the project maintainer (Arjan van de Ven).
      Use `git log --format="%an <%ae>" <prev-stable-tag>.. | sort -u` to get full name+email.

## Tagging the release

> **Agent note:** Only perform this section after explicitly confirming
> with the user that all checks above have passed and they are ready to tag.

- [ ] Confirm with the user that all checks passed and they approve tagging
- [ ] `git tag -a vX.Y -m "Release vX.Y"`
- [ ] `git push origin vX.Y`

