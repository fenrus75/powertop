This repository is for PowerTOP, a linux tool to find the sources of power
consumption in a system.

Do **NOT** update this file to record the results of code review or fixes
for code review comments!

The code review rules for this project are in `review/review.md` and this
includes a style guide in `review/style.md`.

The class hiearchy is documented in `review/class.md` and this document
needs to be kept uptodate as changes to the class hierarchy are made.

Also read `review/tools.md` and `tests/testdesign.md` into the context when
working on the testsuite.


# Global code change process

1. Collect pre-change code coverage on the test suite (or reuse from context)
2. Make the change
3. Build the change (including the test suite) -- requirement: no new warnings
4a. Run the test suite -- requirement: no failure
4b. Collect post-change code coverage on the test stuite
5. Review the change against the intent and the rultes in `review/review.md`
6. Make a git commit with a descriptive commit message 

**Build verification:** never pipe `ninja` through `grep` to filter for
errors/warnings. `grep` returns exit code 1 when it finds no matches,
making a clean build look like a failure. Run ninja directly and read its
full output: `ninja -C <build_dir>`. For a clean-room check use
`meson setup --wipe /tmp/pt_check && ninja -C /tmp/pt_check` (no grep).

# [[maybe_unused]] placement rule

`[[maybe_unused]]` must appear **before** the full parameter declaration,
not after a `*` or `&` qualifier or between type and name.

Correct:   `[[maybe_unused]] struct foo *name`
Correct:   `[[maybe_unused]] const std::string &name`
Correct:   `[[maybe_unused]] int name`
Wrong:     `struct foo *[[maybe_unused]] name`
Wrong:     `const std::string &[[maybe_unused]] name`
Wrong:     `int [[maybe_unused]] name`

GCC warnings triggered by the wrong placement:
- `'maybe_unused' on a type other than class or enumeration definition`
- `attribute ignored`
- `unused parameter`

# Git commit notes

Git commits in this repo must use `--no-gpg-sign` flag to avoid hanging due
to GPG agent unavailability:
```
git commit --no-gpg-sign -F commitmsg.txt
```
Write the commit message to a file in the project directory (NOT /tmp), then
remove it after committing. Use `git add -A` carefully — always check for
untracked files (like DEADJOE, temp files) that should NOT be committed; use
`git restore --staged <file>` to unstage them before committing.
The repo has a global `user.signingkey` set, but the GPG agent is not running
in the terminal session. Setting `commit.gpgsign=false` locally is not
sufficient — `--no-gpg-sign` on the command line is required.

# Derived class serialize tests (tests/base/ and tests/devices/)

Three additional derived classes now have snapshot tests in `tests/base/`:

- `test_process.cpp` — kernel_thread (N cmdline) and user_process (R cmdline).
  Pass `tid == pid` (non-zero) to skip `/proc/status` read; only one fixture
  record per scenario. Link: `process.cpp + powerconsumer.cpp +
  powerconsumer_stubs.cpp + lib.cpp + test_framework.cpp`.

- `test_usb_wakeup.cpp` — enabled and disabled wakeup states.
  Constructor reads nothing; `wakeup_value()` reads `usb_path` once per
  `serialize()` call (via `wakeup::collect_json_fields`). One fixture record
  per scenario. Link: `wakeup_usb.cpp + wakeup.cpp + lib.cpp + test_framework.cpp`.

- `test_sysfs_power_meter.cpp` — charging, discharging_direct, discharging_fallback.
  `end_measurement()` triggers `measure()` which reads: present, status,
  power_now, energy_now (or fallback voltage/current/charge). Fixture record
  order must exactly match the read sequence. Link: `sysfs.cpp +
  measurement.cpp + lib.cpp + test_framework.cpp`.

# collect_json_fields pattern

When adding `collect_json_fields(std::string &_js)` to derived classes:
1. Add declaration to header public section: `void collect_json_fields(std::string &_js) override;`
2. Append implementation to the .cpp file
3. Always call parent's `collect_json_fields(_js)` first
4. Use `JSON_FIELD(x)` for simple fields, `JSON_KV(k, v)` for computed/cast values,
   `JSON_ARRAY(k, vec)` for `vector<T*>` where T has `serialize()`
5. For `vector<string>`, manually build JSON array (no pointer T, can't use JSON_ARRAY)
6. For `nhm_*` and `i965_core` classes: call `abstract_cpu::collect_json_fields(_js)`
   (not the intermediate cpu_package/cpu_core base — those lack the method)
7. Skip non-serializable members: mutex, atomic, pthread_t, pointer-to-interface


# Method tests: write_log, measurement cycles, scheduling

## test_framework write_log

`get_write_log()` returns all writes captured unconditionally in both
record and replay modes. Key behavior in `replay_write()`:
- `write_sequences.count(path) == 0` → no W records provided: silently
  capture in write_log (opt-in verification via test code assertions)
- `write_sequences.count(path) > 0` and queue empty → "TEST FAIL: Extra
  write" (queue exhausted — still an error)
- W records present → validate AND capture in write_log

## sysfs_tunable method tests (tests/base/)

`result_string()` in tunable base calls `good_bad()` internally — which
triggers a sysfs read. Do NOT call `serialize()` after `reset()` if the
object's `good_bad()` will read sysfs: the read will hit the real
filesystem. Either keep replay active during serialize, or test `good_bad()`
and write assertions separately without calling serialize.

`bad_value` is private in `sysfs_tunable` — verify via `serialize()` JSON
or `toggle_bad` (protected in tunable base). Use a derived test class to
expose `toggle_bad`.

## Fixture creation with trace_tool.py add

**Never hand-compute base64 for fixture files.** Use `trace_tool.py add`
(creates the file if it doesn't exist; values auto-encoded to base64;
validate after with `trace_tool.py validate`):

  python3 scripts/test_tools/trace_tool.py add FILE R /some/path "plain text value"
  python3 scripts/test_tools/trace_tool.py add FILE D /some/dir entry1 entry2
  python3 scripts/test_tools/trace_tool.py add FILE N /missing/path
  python3 scripts/test_tools/trace_tool.py add FILE T 10 500000
  python3 scripts/test_tools/trace_tool.py add FILE A /sys/foo 4 0
  python3 scripts/test_tools/trace_tool.py add FILE M 0 611 deadbeef   # cpu offset [value], default value 0

For D records: entries are space-separated, sorted and newline-joined
before base64 encoding; empty value → empty/missing directory record.
M records in replay mode: `replay_msr()` returns 0 (success), but
`rapl_interface.cpp` domain detection checks `ret > 0`, so MSR-path
domains are never detected via replay — use M records for "no domain"
fallback tests; prefer the powercap path for domain-present tests.

Other useful subcommands: `list --content FILE` (decoded values),
`list --path SUBSTR FILE` (filter), `extract`/`replace FILE LINE
file.txt` (decode/replace a record's content), `edit FILE LINE` (open
in `$EDITOR`, default `joe`), `search FILE REGEX` (grep paths),
`validate FILE` (check format).

## Test infrastructure: stub files

When a class pulls in a heavy dependency chain through one function:
- Create `tests/devices/stub_display.cpp` for `create_tab` / `get_ncurses_win`
- Create `tests/base/stub_runtime_pm.cpp` for `device_has_runtime_pm`
Both stubs inline the actual logic using intercepted `read_sysfs()` /
`read_file_content()` — so the stubs remain correctly intercepted in tests.

## Coverage workflow

`scripts/coverage_report.sh [label] [build_dir]` captures a named lcov
snapshot from the coverage build directory (default: `build_cov`).

One-time setup: `meson setup build_cov -Db_coverage=true -Denable-tests=true`

Before/after pattern:
  ninja -C build_cov test && scripts/coverage_report.sh before
  # add test, rebuild
  ninja -C build_cov test && scripts/coverage_report.sh after

`ninja coverage` is broken due to duplicate test_framework.cpp symbols;
use the script instead (it passes `--ignore-errors inconsistent`).

When running the interactive `build_acov/powertop` smoke/leak checks after
`ninja -C build_acov test`, gcov may print
`libgcov profiling error: ...gcda:overwriting an existing profile data with a different checksum`.
Treat this as coverage-profile noise from the coverage-instrumented binary, not
as an ASAN/runtime failure by itself.

`tests/lib/powertop-test-lib` links `src/lib.cpp` without `src/display.cpp`.
If `lib.cpp` calls display helpers such as `set_notification()`, keep the
reference optional (for example via a weak declaration) so non-UI test targets
still link.

# RAII / vector<T*> ownership status (migration complete — see RAII log above)

**Non-owning observer vectors (raw pointer correct — do not migrate):**
- `all_power` — aggregates from all owning collections; never deletes.
- `all_cpus` — flat indexed lookup into `system_level.children`; may contain nullptrs.
- `cpudevice::child_devices`, `i915_gpu::child_devices` — views into `all_devices`.

Notes from the completed migration:
- `measurement_manager.cpp`: the factory function
  `extech_power_meter(const std::string &)` collides with the class name;
  disambiguate with `new class extech_power_meter(...)`.
- `perf_bundle::records` (`vector<void*>`, malloc/free) is the sole
  remaining exception — see RAII log above.

# NaN / division-by-zero guard patterns

Two thresholds are used to guard floating-point divisions:

- `measurement_time < 0.00001` (10 µs, seconds unit) — used in
  `do_process.cpp` `total_*()` functions and `powerconsumer.cpp`.
- `time_factor < 1.0` (1 µs, microseconds unit) — used in
  `fill_cstate_line()` / `fill_cstate_percentage()` in
  `cpu_core.cpp`, `cpu_linux.cpp`, `cpu_package.cpp`, and
  `i965_core::fill_cstate_line()` in `intel_gpu.cpp`.
- `time_delta < 1.0` (1 µs) — same as time_factor, used in
  `intel_gpu.cpp`.

`percentage()` in `lib.cpp` clamps negatives to 0 but does NOT cap
at 100 (commented out), and does NOT trap NaN — callers must guard
their divisors.

**D record format:** `D path base64(newline-joined-entries)`
- Empty base64 → empty/not-found directory (returns `{}`)
- In `trace_tool.py add`: `add FILE D /some/dir "entry1 entry2"` — entries are
  space-separated, sorted then newline-joined before base64 encoding.

**Note:** `byt_has_ahci()` uses `std::filesystem::exists()` rather than `list_directory()` since it only checks directory existence. The `DIR *dir` member was removed from `intel_util` class.

**Empty D record format:** `D path` (no trailing b64 token) = empty/missing directory.
`test_framework.cpp` and `trace_tool.py` both handle this correctly.

# Release checklist process

See `release-checklist.md` for the full pre-release checklist. Key points:
- All five build types must be **clean builds** (`meson setup --wipe`) with zero warnings (absolute, not just "no new warnings")
- All five build types need `-Denable-tests=true` for full test suite
- ASAN build: `build_acov` (-Denable-tests=true -Db_coverage=true -Db_sanitize=address)
- gcov build: `build_cov` (-Denable-tests=true -Db_coverage=true)
- Release notes go in `doc/relnotes.md`; README.md has a "Recent releases" table to update
- Version is in `meson.build` `version:` field
- Tagging requires explicit user confirmation after all checks pass
- **Never pipe ninja through grep** in the checklist or anywhere else — this is now documented in release-checklist.md as well
- POTFILES.in audit: `comm -23 <(grep -rl '\b_(\|N_(' --include="*.cpp" --include="*.h" src/ | sort) <(grep -v '^#\|^$' po/POTFILES.in | sort)` — run this at every release; new source files often have translatable strings that are missing
- msgmerge only runs for languages listed in `po/LINGUAS` (11 active languages); the other .po files are not built but still exist

# Time formatting pattern (lib.cpp / lib.h)

`get_time_string(const std::string &fmt, std::chrono::system_clock::time_point tp)`
is declared in `lib.h` and defined in `lib.cpp`. It converts to local time via
`std::chrono::zoned_time{std::chrono::current_zone(), tp}` and formats using
`pt_format("{:" + fmt + "}", zt)` — strftime-style specifiers (`%c`, `%Y%m%d-%H%M%S`).
Call sites use `std::chrono::system_clock::now()` instead of `time(nullptr)`.


## Coverage baseline (build_acov, test suite + one --once run)

**v2.16:** lines: **70.0%** (6940/9908)  functions: **76.5%** (811/1060)
Snapshot saved to: `/tmp/pt_v2.16_html/`

Previous (commit 8906999): lines: 69.7% (6331/9078)  functions: 75.4% (741/983)

# Valgrind memory leak policy

Run: `sudo valgrind --leak-check=full --show-leak-kinds=all build/powertop --html --time=3`

Known remaining "out of scope" library-internal leaks (do not fix):
- 20 bytes: libtracefs internal `strdup` of tracing dir path (no public cleanup API)

Fixed leaks (commit 7791a45):
- `end_pci_access()`: replaced `pci_free_name_list()` with `pci_cleanup()` + null pointer
- `clear_tuning()`: now deletes `tune_window` before clearing tunables vectors
- `clear_wakeup()`: now deletes `newtab_window` before clearing wakeup_all vector

Policy: 3rd-party library leaks are out of scope UNLESS caused by us not calling APIs correctly.

# Fixing review comments and bug reports

First create a testcase for the issue when possible at all; this test case will fail at
first (to show that the issue is real). After the issue is fixed, the test
should then pass.

# tuning.cpp source ordering convention

`init_tuning()` is organized into three clearly separated blocks (source order only;
`sort_tunables()` still handles runtime order):
1. **Dynamic helpers** (device-enumerated): `add_bt_tunable`, `add_i2c_tunables`,
   `add_runtime_tunables("pci")`, `add_sata_tunables`, `add_usb_tunables`, `add_wifi_tunables`
   — sorted alphabetically by function name.
2. **String/categorical `add_sysfs_tunable` calls** — sorted alphabetically by description.
3. **`add_numeric_sysfs_tunable` calls** — sorted alphabetically by description.

When adding a new tunable, place it in the correct block in alphabetical order.

## GPU tab design pattern (data vs. display separation)

Measurement data lives in data classes, not in the display layer.
Follow this pattern:
- Add public result vectors to `xe_core` (or other data class): `gt_labels`, `per_gt_busy_pct`
- Compute in `measurement_end()` using the before/after timestamps already present
- Expose a singleton accessor `get_xe_core()` backed by a static pointer set in the constructor
- Display functions call the accessor and read from it — no static tracking, no sysfs re-reads

`per_gt_busy_pct[i]` is initialized to `-1.0` and left negative until the first complete
measurement cycle. Display code must skip negative values ("no data yet").

`gt_labels` are populated once at construction time from `find_xe_gt_idle_paths()`, which
now fills both the paths vector and the labels vector in a single pass (parallel by design).

# test_framework debug mode

Set `PTTEST_DEBUG=1` in the environment before running a test binary to print
every intercepted call to stderr with `[pttest]` prefix (path, type, result).
No recompile required — detected via env var in the constructor.
Use this to diagnose which records are missing from a fixture.

# Calibration DPMS control (issue #110)

`calibrate.cpp` uses a two-tier display power control:
1. `sysfs_dpms_set(state)` — writes `state + "\n"` to each enabled DRM connector's
   `/sys/class/drm/card*/card*-*/dpms` attribute; works on X11, Wayland, headless.
   Returns `true` if at least one connector was written.
2. `set_dpms(state)` — calls sysfs first; if that returns false, falls back to
   `xset dpms force {state} >/dev/null 2>&1` (no hardcoded `DISPLAY=:0`).

Brightness calibration already uses sysfs (`/sys/class/backlight/*/brightness`)
directly — no display-server dependency there.

The connector discovery mirrors `backlight.cpp:dpms_screen_on()`:
outer loop finds `card0`/`card1` (no '-'), inner loop finds `card0-HDMI-A-1` (has '-').

# Systematic code review workflow (file-by-file from a git tag)

To review all changes since a tag:
1. `git diff --name-only v2.16-rc2 HEAD` — get changed file list
2. Load into SQL `review_queue` table with status 'pending'
3. Process one file at a time using `git diff v2.16-rc2 -- <file>` + `view` of current file
4. Immediately fix style violations; propose structural issues to user as choices

Style violations to fix immediately (no user confirmation needed):
- C-style casts: `(T)x` → `static_cast<T>(x)`
- Ternary operators: replace all with if-statements (style.md §1.4)
- `std::format(_("..."))` → `pt_format(_("..."))` (pt_format rule — translated strings need pt_format)
- Spacing: `!=0)` → `!= 0)`, `){` → `) {`

Structural issues to propose to user (require confirmation):
- Dead conditions (always-true/false guards)
- Unused variables or accumulators
- Unsigned integer wraparound risk before floating-point cast

After all files done: run `ninja -C build_tf test` (60/60), then commit with detailed message.


# turbostar MCP tool notes (consolidated, through 2026-10-09)

- `fs_compile_project`/`fs_run_tests` target a hardcoded `build` dir path
  that may not match this checkout; pass `clean: true` to force a real
  rebuild, and fall back to `ninja -C <build_dir> [test]` via bash when
  they report a stale/missing build-data error — this has been the
  reliable path for non-default dirs like `build_tf`/`build_acov`.
- `fs_grep_files`'s `pattern` is literal by default; always set
  `is_regex: true` for `|`/`.*`/character-class patterns or it silently
  returns "No matches found".
- `fs_replace_content` reports "Successfully replaced" even on a true
  no-op (identical target/replacement) — verify with a follow-up
  `view`/diff.
- turbostar's crash catcher intercepts crashes (including `assert()`
  SIGABRT) from anything run via its process tools, not just the primary
  app; check `crashdump_list`/`crashdump_get_info` on any unexpected
  failure instead of assuming a bare test failure. `crashdump_get_info`
  caches by crash signature — call `crashdump_clear` before re-verifying a
  fix. For local/arg values inline, use `agent_debug_coredump` + `bt full`.
- `run_cpp`'s execution step has intermittently failed with exit 127 even
  though the compiled binary is present on disk; if it recurs, fall back
  to writing a temp `.cpp` and invoking `g++`/running it via bash.
- Device display convention: a `device` subclass meant purely for
  parameter/power accounting (not shown in Device stats) should override
  `show_in_list()` to return `false` (see `cpudevice`, `i915gpu`, `xe-gpu`,
  `rfkill`); both report loops in `device_manager.cpp` already honor it.
- Before committing, always check `git status`/`git diff --staged` for
  pre-existing staged changes unrelated to the task, and stage only the
  specific files the task touched — never `git add -A` in this repo
  (recurring leftover clutter: `review-pr-*.md`, `crash.c`,
  `pick_random_files.py`, `.mcp.json`, stray `*~` backups).

# PR review workflow notes (2026-09-15)

- `gh` CLI works directly for PR review here. Useful: `gh pr view <n>
  --json ...`, `gh pr diff <n>`, `gh pr checks <n>`, `gh pr merge <n>
  --merge --delete-branch=false`.
- To build/test a specific PR without disturbing the main worktree:
  `git fetch origin pull/<n>/head:pr-<n>` + `git worktree add /tmp/pr<n>
  pr-<n>`, build in a separate meson dir there, then `git worktree remove
  /tmp/pr<n> --force && git branch -D pr-<n>` to clean up.
- Per-PR review reports are saved as `review-pr-<n>.md` (not the generic
  `review.md`) so multiple PR reviews in one session don't overwrite each
  other.
- `fs_read_lines`/`fs_file_codemap`/`fs_read_symbol` are preferred over
  `view`/bash `cat`: they add line numbers and an auto-appended symbol
  codemap.
- User preference: `EXIT_FAILURE`/`EXIT_SUCCESS` over bare `exit(1)`/
  `exit(0)`, and `std::from_chars` over `strtol`/`strtoul`/`atoi` for
  numeric CLI argument parsing.
# RAPL stub fail-injection pattern for uninitialized-value regressions (2026-09-15)

- `tests/devices/stub_rapl_iface.cpp` supports simulating "domain present
  but this one read call fails" via `rapl_fail_next_pp0_energy()` /
  `_dram_energy()` / `_pp1_energy()` — one-shot flags making
  `get_*_energy_status()` return -1 **without writing `*s`**, matching
  the real interface's failure semantics (needed to test PR #223's "leaves
  output untouched on failure" contract).
- Regression-test pattern: queue constructor/`start_measurement()` values,
  call `rapl_fail_next_pp1_energy()` right before the call under test,
  then assert power is deterministically `0.0` and `last_energy` is
  unchanged from the last successful reading. Verify the test actually
  catches the bug by reverting the fix and running under
  `valgrind --track-origins=yes` (should show both an assertion failure
  and "Uninitialised value" errors), then restore the fix.
- Added `valgrind-rapl-devices` to the meson `valgrind` test suite so this
  class of regression is always caught by `meson test --suite valgrind`.
  Variables from `subdir('tests')` are visible in the top-level
  `meson.build` after `subdir()` returns (Meson shares scope like a
  textual include) — no need to duplicate `valgrind_common_args` per subdir.

# std::from_chars gotcha: no leading '+' for floating point (2026-09-15)

- Unlike `strtod`/`strtol`, `std::from_chars` for **floating-point**
  types does NOT accept a leading `+` sign (only `-` is recognized
  outside an exponent, per the C++ standard's grammar for
  `from_chars`). If replacing `strtod()` with `from_chars()` and the
  input string may be explicitly `+`-prefixed (e.g.
  `src/measurement/extech.cpp`'s BCD-decoded values, which
  `decode_extech_value()` always prefixes with `+` or `-`), skip a
  leading `+` manually before calling `from_chars()`, or the call will
  fail with `ec != std::errc()` for every positive value. This bit us
  when modernizing `extech.cpp`'s `strtod()` call — caught it with a
  quick standalone g++ probe before committing, not by the test suite
  (no existing unit test covers `parse_packet()`'s numeric parsing).
  Integer `from_chars` does not have this restriction and works as a
  drop-in replacement for `strtol`/`strtoul`/`strtoull` when the
  input has already been range/format validated.
- When modernizing a `strto*()` call whose input isn't obviously
  strictly numeric (e.g. `/proc/<name>` directory entries, which
  include non-pid pseudo-entries like "sys"/"net"), prefer treating a
  failed/partial `from_chars` parse as "reject this entry" rather than
  the old silent-zero behavior of `strtoull` on invalid input — this
  is both more correct and matches the project's general preference
  for explicit validation over silent fallback to 0/garbage.

# Random-file-audit workflow ("random file audit Friday")

- `pick_random_files.py` (repo root) walks the tree for `.cpp`/`.c` files
  (excluding `.git`, `build*`, `subprojects`), and prints a true-random
  sample via `random.sample()` with an OS-entropy seed (`random.seed()`
  with no argument). Run as `python3 pick_random_files.py [N]` (default
  N=5). Reuse this script for future audit-Friday sessions instead of
  rewriting it.
- The `turbostar` MCP server's `code_review` tool family (activate with
  `turbostar-activate_tool_family name=code_review`) provides a proper
  review-item database: `create_code_review_item`, `list_code_review_items`,
  `resolve_code_review_item`, `confirm_code_review_item`. Prefer filing
  findings there (with `path`/`line_number`/`severity`) over only writing a
  markdown report, so items can be tracked to resolution across sessions.
  `turbostar-security_scan_c` (cppcheck) is a good zero-cost extra pass to
  run alongside manual review.

# unique_ptr conversion pattern for a single owning raw pointer member

When migrating a class's single owning `T *member` (raw pointer, `new`/
`delete`) to `std::unique_ptr<T>` (e.g. `report_maker::formatter` — see
commit fixing review item #1, report-maker.cpp/.h):
- Reassigning a `unique_ptr` (`member = std::make_unique<Derived>()`)
  automatically destroys whatever it previously held — drop any
  unconditional `delete member;` that used to precede the reassignment
  logic. This also fixes "unhandled branch leaves a dangling pointer"
  bugs for free: if no branch reassigns, the old (still valid) object
  is simply kept instead of ending up deleted-but-referenced.
- If the header only forward-declares the pointee (`class T;`), do
  **not** write `~Owner() = default;` inline in the header — `unique_ptr`'s
  deleter needs a complete type at the point the destructor is
  instantiated. Declare `~Owner();` in the header and define
  `Owner::~Owner() = default;` in the .cpp file, which includes the
  full definition.
- Check for test stubs that independently reimplement the owning
  class's constructor/destructor (e.g. `tests/devices/stub_report.cpp`
  defining its own `report_maker::report_maker`/`~report_maker`) —
  these must be updated in lockstep or you'll get "explicitly-defaulted
  destructor" redefinition errors or stale `delete`-on-non-pointer
  compile errors.
- Verify with the `valgrind` meson test suite (`meson test -C build`),
  not just the plain unit tests — it's the cheapest way to confirm the
  new ownership doesn't leak or double-free.

# RAII migration progress log (condensed; migration complete)

All owning `vector<T*>`/`map<K,T*>` containers and single owning raw
pointers in `src/` have been converted to `unique_ptr`-based ownership
(completed 2026-09-25 through 2026-10-07): `power_meters`, `all_devfreq`/
`devfreq::dstates`, `perf_bundle::events`, `all_interrupts`,
`all_processes`, `all_proc_devices`, `all_tunables`/`all_untunables`,
`abstract_cpu::children`/`cstates`/`pstates`, `all_devices`,
`past_results`, `all_timers`, `all_work`, `tab_windows`, `wakeup_all`,
and `devlist.cpp`'s `one`/`two`/`devpower`. The ownership map earlier in
this file lists current owning-vs-observer status per container.
Exception: `perf_bundle::records` (`vector<void*>`) still uses raw
`malloc`/`free` — needs a custom RAII wrapper, tracked separately.

**Recurring gotchas, reusable for any future `vector<T*>`→`unique_ptr`
container migration:**
- Grep test files for `container.push_back(&local_var)` before
  converting — a stack address pushed into an owning container crashes
  on teardown once the destructor calls `delete` on every element; fix
  by switching the mock to `std::make_unique<T>(...)`.
- A test file with its own local stub for a global owning container
  (instead of linking the real class's `.cpp`) will fail to link
  ("undefined reference to vtable for X") once that container owns via
  `unique_ptr`, because the stub's destructor now needs the class's
  vtable — add the real class's `.cpp` to that test's `meson.build`
  sources and remove the local stub.
- `std::for_each` over a `map` needs a callback matching its
  `value_type` exactly (including `const` on the key).
- Factory functions colliding with a class name (e.g.
  `extech_power_meter(const std::string &)` vs `class extech_power_meter`)
  need `new class extech_power_meter(...)` to disambiguate.
- For "owning map with an observer needed mid-init" (e.g. `tab_windows`):
  grab a raw observer pointer from the `unique_ptr` *before* moving it
  into the owning container, since fields depending on later init data
  (e.g. `cursor_max`) can only be set after the container owns the
  object. Prefer a single `get_current_tab()`-style accessor over
  repeating the index→name→owner lookup inline at every call site.

# display.cpp cursor navigation pitfall

`cursor_down()` in `src/display.cpp` computes a scroll cap
`ypad_max = content_max_y - viewport_height + 2` (clamped to >=0) to
limit how far the pad scrolls. For tabs whose content fits entirely in
the viewport (e.g. WakeUp, and Tunables when the list is short),
`ypad_max` legitimately evaluates to 0. Never nest the `w->cursor_down()`
call (which just moves `cursor_pos` and repaints) inside an
`if (w->ypad_pos < ypad_max)` guard meant only for the scrolling
`prefresh()` call — doing so silently breaks arrow-key navigation on
short-content tabs while leaving longer-content tabs looking fine,
which is exactly what happened in issue #217 (fixed by commit
3551819). When touching this function, keep cursor-position movement
unconditional and gate only the scroll `prefresh()` on the ypad
bounds check.

# Release checklist walkthrough notes (v2.16.1, 2026-10-07)

- Before starting, `meson.build` and the latest git tag were already both
  `v2.16.1-rc1` with 47 more commits merged since that rc tag — i.e. an rc
  had been cut but not yet promoted to stable, and plenty of new work had
  landed on top. Always check `git tag --sort=-v:refname | head` *and*
  `meson.build`'s `version:` field *and* `git log <latest-tag>..HEAD
  --oneline | wc -l` before asking the user which version to cut; don't
  assume the previous tag matches `meson.build` or that there's nothing new
  to release.
- When promoting an existing `-rcN` cycle straight to stable (skipping
  further rcs), the release-notes base for `git shortlog`/tunable-diff
  purposes is still the **last stable tag** (e.g. `v2.16`), not the rc tag
  — the `doc/relnotes.md` section for the rc is renamed in place and
  extended with everything merged since the rc, exactly like the existing
  RC-cadence rule in `release-checklist.md` describes for rc→rc moves.
- After a clean `meson setup --wipe` + `ninja -C build_acov test` run
  followed by a `sudo build_acov/powertop --once` coverage run, the
  `*.gcda`/`*.gcno` files under `build_acov` end up root-owned; run
  `sudo chown -R $(whoami):$(id -gn) build_acov` before
  `scripts/coverage_report.sh`, or lcov will fail/warn on permission
  errors reading them.
- The checklist's valgrind/ASAN leak-check steps and the `--once` coverage
  run all require `sudo` (binding to real hardware sysfs paths as root);
  run these via `sudo -E` to preserve `ASAN_OPTIONS` and other env vars
  needed by the binary.
- Before `git add`ing the release commit, always check for pre-existing
  unrelated untracked files in the repo root (this session had leftover
  `review-pr-*.md`, `crash.c`, `pick_random_files.py`, `.mcp.json`, etc.
  from earlier sessions) — stage only the specific files the release
  touched (`meson.build`, `doc/relnotes.md`, `README.md`, `po/*`), never
  `git add -A`, to avoid sweeping in unrelated clutter.
- Full release cycle (5 clean builds + 5 test suites + 2 leak checks +
  coverage capture) took about 25-30 minutes of wall-clock tool time in
  this sandbox; budget accordingly and run steps serially since they share
  the same `sudo`/build-directory state (parallelizing the builds is safe,
  but the two `sudo powertop` leak/coverage runs and valgrind should not
  overlap with each other).

# Corporate C++ modernization protocol session (2026-10-09)

Ran `protocols/cpp-modernization.md` (from `/home/arjan/git/rules/rules/`)
Mode 4 against this repo for the first time. Key facts for future runs:

- PowerTOP has **no external ABI surface**: `meson.build` only declares one
  `executable()` target (no `shared_library()`, no installed headers), so
  the protocol's wrapper-preservation step (Phase 2 step 4) never applies
  here — signatures can change in place.
- Baseline scan commands that worked in this sandbox:
  `cppcheck --enable=all --inconclusive --std=c++20 -I src src/` and, from
  a build dir with `compile_commands.json` (e.g. `build_tf/`),
  `run-clang-tidy -checks='-*,modernize-*' -quiet 'src/.*\.cpp$'`. Both
  `cppcheck` and `clang-tidy`/`run-clang-tidy` are preinstalled.
  `clang-tidy --list-checks -checks='modernize-*'` (without the leading
  `-*,`) silently lists clang-analyzer checks instead — always prefix with
  `-*,`.
- Backlog lives in **`MODERNIZATION-PLAN.md`** (repo root, committed, not
  ephemeral like `review-*.txt`). It's tiered A (security-critical:
  strncpy/cstyle casts/sign-comparison/C arrays) / B (RAII-lifetime: naked
  `new`, scoped locks, member init) / C (idiomatic: override, explicit,
  auto, etc.), with per-item status/commit columns. Re-read it before
  resuming a future modernization pass instead of re-running the scans.
- `abstract_cpu::children`, `all_devices`, `wakeup_all`, `all_tunables` /
  `all_untunables` are **already** `vector<unique_ptr<T>>` (prior RAII
  migration work) — the remaining `No Naked New` backlog items (B1-B3) are
  about the raw `new X(...)` *construction idiom* at call sites that
  already end up wrapped (`std::unique_ptr<T>(ptr)` or pushed into an
  owning vector), not about un-migrated ownership.
- Found and fixed (commit `186a270`): `pt_strcpy()` in `src/lib.h` used
  `strncpy()` + manual `d[N-1]='\0'`; replaced with `snprintf(d, N, "%s",
  s)` (same truncate+NUL-terminate behavior, now the destination capacity
  bounds the copy instead of relying on a manual fixup) and routed the two
  remaining direct `strncpy()` call sites (`ethernet.cpp` ifreq, `bluetooth.cpp`
  hci_dev_info) through the helper. Added pinning regression tests in
  `tests/lib/test_lib_pure.cpp` before changing the implementation
  (short / exact-fit / truncating / empty-string cases).
- `src/lib.h`'s `#ifdef __cplusplus` region needed an explicit `#include
  <cstdio>` added (near the existing `<chrono>`/`<ctime>` block around
  line 79) — `snprintf` wasn't otherwise transitively included there.
- When asking the Phase 0 "C++20 or C++23" / ABI / ASan questions via
  `ask_user`, bring concrete scan findings into the question text per the
  protocol ("bring findings to the ask... rather than asking blind") —
  users can answer much faster against findings than against an abstract
  question.

## C++ modernization protocol run: Tier C summary (through Phase 3, 2026-10-09)

- Completed all of Tier C (C1-C9) and Phase 3 final verification. All
  Tier A/B/C backlog items in `MODERNIZATION-PLAN.md` are `done` or
  justified-excluded; labeled-rule `modernize-*` findings across `src/`
  are down to 0. Test suite stayed at 61/61 throughout (grew from the
  Phase 0 baseline's 60/60 — a test was added earlier in the session).
- **Backlog site-count estimates from Phase 1 were frequently wrong in
  both directions** once a fresh clang-tidy/cppcheck scan was run
  immediately before fixing: C1 override 20→268, C4 const-qualifiers
  42→31, C9's `modernize-use-ranges` 14→29. Always re-scan right before
  fixing a backlog item instead of trusting the Phase 1 count; document
  any discrepancy in the commit message.
- `run-clang-tidy -p <builddir> -checks='-*,<check-name>'` then
  `grep -n "warning: .*\[<check-name>\]"` is the reliable per-check scan
  pattern. For combined multi-check scans, each check needs its own
  `warning:.*\[name\]` alternation in the grep — a single bracket-group
  alternation (`\[a|b|c\]`) does not match correctly.
- `modernize-use-override` also flags a *redundant* `virtual` keyword on
  a method that already has `override` (not just a *missing* `override`)
  — worth a second, narrower grep (`"redundant since.*already declared"`)
  after the main override pass, since the main pass's grep pattern won't
  catch this variant.
- `[[nodiscard]]` is not inherited by overrides in C++; don't extend a
  nodiscard backlog item into base-class headers just because a test
  mock overrides an un-annotated virtual — that's out of scope unless
  the task says to touch the base class.
- Pass-by-value+`std::move` only applies to constructor params *copied
  into a member in the initializer list*; params merely *assigned* in
  the constructor body are correctly left as `const &` (clang-tidy's
  `modernize-pass-by-value` itself won't flag these).
- `std::ranges::remove(container, val)` returns a subrange; use
  `.begin()` on it with `container.erase(...)`, same as the classic
  erase-remove idiom but without the second `container.end()` argument
  to `remove`.
- `std::filesystem::directory_iterator` satisfies the C++20 range
  concept directly (ADL-found `begin()`/`end()`), so
  `std::ranges::transform(it, ...)` works without the explicit
  `std::filesystem::begin(it)/end(it)` calls some code used historically.
- `using Name = struct __attribute__((packed)) { ... };` is valid and
  preferred over `typedef struct {...} __attribute__((packed)) Name;`
  for attribute-placement-sensitive typedef→using conversions.
- `cppcheck_out.txt` is a local scratch artifact, never tracked in git —
  do not `git add -A` or otherwise let it slip into a commit; it was
  caught and removed via `git rm --cached` + `commit --amend` once
  already.
- cppcheck's `--enable=all` finding set for const-qualifiers
  (`constParameterPointer`/`constVariableReference`/etc.) is scan-scope
  sensitive: a Phase 3 re-scan surfaced 16 new residual hits in files
  C4 never originally touched (`src/perf/perf_event.h` — a kernel-API
  mirroring header where pointer params intentionally match upstream
  kernel signatures and should NOT be made const; `src/perf/perf.cpp`,
  `src/measurement/measurement_manager.cpp`). These are documented as
  future-session findings in `MODERNIZATION-PLAN.md`, not a regression.
- clang-tidy `clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling`
  flags every `fprintf`/`printf` (e.g. `src/tuning/iw.c`) suggesting Annex K
  `fprintf_s`, which glibc lacks. False positive for literal formats; suppress
  the check rather than change code. `src/tuning/iw.c` has no unsafe string
  functions; its only real issue is the unchecked `nl_recvmsgs()` loop (~l.255).

## PCI name lookup (libpci removed)
- libpci is no longer used; `src/lib.cpp` parses pci.ids itself (lazy, cached in maps, searched in /usr/share/hwdata, /usr/share/misc, /usr/share).
- `-DHAVE_NO_PCI` is now only a test switch: it stops the host pci.ids from being loaded. Tests feed fixtures via `pci_ids_parse(std::istream&)`.
- Decision recorded: user chose "own parser, remove libpci" over a meson option; the valgrind suppression and 21-byte leak exception were dropped.
