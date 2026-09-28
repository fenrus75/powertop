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

Use `trace_tool.py add FILE R PATH VALUE` to build fixture files from
scratch (creates file if it doesn't exist). Values are plain strings,
auto-encoded to base64. Validate after building with `trace_tool.py validate`.

Example for backlight (2 reads in start_measurement):
  trace_tool.py add backlight_start.ptrecord R /sys/class/backlight/lcd/max_brightness 100
  trace_tool.py add backlight_start.ptrecord R /sys/class/backlight/lcd/actual_brightness 60

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

# RAII / vector<T*> ownership map

A full review of all `std::vector<T*>` in `src/` is in `raii.md`.

**Owning vectors (should migrate to `vector<unique_ptr<T>>`):**
- `power_meters`, `all_tunables`, `all_untunables`, `all_devices`, `past_results`,
  `wakeup_all`, `all_interrupts`, `all_processes`, `all_proc_devices` (global)
- `abstract_cpu::children`, `abstract_cpu::cstates`, `abstract_cpu::pstates` (member)
- `perf_bundle::events`, `devfreq::dstates` (member)

**Non-owning observer vectors (raw pointer correct — do not migrate):**
- `all_power` — aggregates from all owning collections; never deletes.
- `all_cpus` — flat indexed lookup into `system_level.children`; may contain nullptrs.
- `cpudevice::child_devices`, `i915_gpu::child_devices` — views into `all_devices`.

**Special:** `perf_bundle::records` — `vector<void*>` with malloc/free; needs custom
RAII wrapper rather than `unique_ptr<T>` (see `raii.md`).
- `wakeup_all` is now `std::vector<std::unique_ptr<wakeup>>`; use
  `std::make_unique<>` at registration sites, `wakeup_all.clear()` for teardown,
  and `.get()` only where a raw `wakeup *` is temporarily needed.
- `all_tunables` / `all_untunables` are now `std::vector<std::unique_ptr<tunable>>`;
  registration sites should use `std::make_unique<>` plus `std::move()` for
  conditional insertion, `clear_tuning()` should just call `.clear()`, and UI/
  test code should use `.get()` or container resize/clear rather than manual
  `delete` loops.
- `measurement_manager.cpp`: the factory function `extech_power_meter(const std::string &)`
  collides with the class name; when constructing the class inside that function,
  use `new class extech_power_meter(...)` (or `emplace_back(new class ... )`) to
  disambiguate the type name from the function.

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

# trace_tool.py M record support

`scripts/test_tools/trace_tool.py add` now supports M (MSR) records:
  trace_tool.py add file.ptrecord M 0 611 deadbeef   # cpu offset [value]
  Default value is 0. `list --content` shows cpu/offset/value for MSR records.
  M records in replay mode: replay_msr() returns 0 (success), but note that
  rapl_interface.cpp domain detection checks ret > 0, so MSR-path domains will
  never be detected via replay (returns 0, not 8). Use for "no domain" MSR
  fallback tests; the powercap path is preferred for domain-present tests.

# ALWAYS use trace_tool.py to create/edit .ptrecord fixtures

**Never hand-compute base64 for fixture files.** Use `trace_tool.py add`:

  python3 scripts/test_tools/trace_tool.py add FILE R /some/path "plain text value"
  python3 scripts/test_tools/trace_tool.py add FILE D /some/dir entry1 entry2
  python3 scripts/test_tools/trace_tool.py add FILE N /missing/path
  python3 scripts/test_tools/trace_tool.py add FILE T 10 500000
  python3 scripts/test_tools/trace_tool.py add FILE A /sys/foo 4 0
  python3 scripts/test_tools/trace_tool.py add FILE M 0 611 deadbeef

Other useful subcommands:
  list --content FILE       — show all records with decoded values
  list --path SUBSTR FILE   — filter by path substring
  extract FILE LINE out.txt — decode a record's content to a file
  replace FILE LINE in.txt  — replace a record's content from a file
  edit FILE LINE            — open a record's content in $EDITOR (default: joe)
  search FILE REGEX         — grep paths by regex
  validate FILE             — check format is valid

For D records: `add` takes space-separated entry names; they are sorted and
newline-joined before base64 encoding. An empty value → empty directory record.

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
- 21 bytes: libpci `pci_lookup_name` internal buffer not freed by `pci_cleanup()` (libpci bug)

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


# turbostar MCP tool notes (2026-09-01)

- `fs_compile_project` without `clean: true` just runs `meson compile`; if a
  `ninja clean` was run out-of-band (or in a different shell) it can report
  "no work to do" without indicating anything is stale. Pass `clean: true`
  to force a real rebuild when in doubt.
- `fs_run_tests` targets a hardcoded `build` dir path and failed with
  "No such build data file" in this environment even though that `build`
  dir was configured/compiled fine via `fs_compile_project`. Falling back
  to `ninja -C build_tf test` via bash worked (60/60 pass). Don't rely on
  `fs_run_tests` here for now — use the `build_tf` test-framework build
  directly.
- Device display convention: a `device` subclass that should exist purely
  for parameter/power accounting (not shown in Device stats) should
  override `show_in_list()` to return `false`, matching `cpudevice`,
  `i915gpu`, `xe-gpu`, and now `rfkill`. Both loops in
  `device_manager.cpp` (`report_devices`, `show_report_devices`) honor
  `show_in_list()`, so new hidden-by-default devices don't need further
  changes there.
- Before committing, always check `git status`/`git diff --staged` for
  pre-existing staged changes unrelated to the task — a stale staged
  deletion of `Makefile.am` almost got swept into an unrelated commit here
  because it was already in the index before `git add <specific paths>`
  was run.
- turbostar's crash catcher intercepts every crash of anything run via its
  process tools (`agent_start_app`, and once fixed, `fs_run_tests`) — not
  just the app under test. A crashing unit test therefore won't just show
  up as a bare test failure; check `crashdump_list` / `crashdump_get_info`
  whenever a run reports an unexpected failure, since a full backtrace
  (with source file/line when built with debug symbols) is likely already
  captured and can save a manual gdb/coredumpctl session.
- This also catches `assert()` failures (SIGABRT), not just segfaults —
  the crash dump includes the assertion expression/message itself, so a
  failing `assert(x == y)` in a test shows up as a proper crash report,
  not just a bare abort trace. Treat assertion-failure test output the
  same way: check the crash dump instead of only reading stdout/stderr.
- `crashdump_get_info` crash IDs are cached per unique crash signature;
  rerunning the exact same crash without an intervening `crashdump_clear`
  can return the *same* crash_id from a previous run rather than a fresh
  one — call `crashdump_clear` first if you need to confirm a fix produced
  a genuinely new (or no) crash.
- With debug symbols, `crashdump_get_info` resolves user frames to
  `file:line` and adds a "Codemap Summary" table of function ranges, but
  still doesn't show the crashing source line or the faulting argument
  value inline — use `agent_debug_coredump` + `bt full` to get local/arg
  values (e.g. `foo(c=0x0)`) in one shot when the concise report isn't
  enough.

# PR review workflow notes (2026-09-15)

- `gh` CLI works directly for PR review here — no separate web access
  needed. Useful commands: `gh pr view <n> --json ...`, `gh pr diff <n>`,
  `gh pr checks <n>`, `gh pr merge <n> --merge --delete-branch=false`.
- To build/test a specific PR without disturbing the main worktree, use
  `git fetch origin pull/<n>/head:pr-<n>` + `git worktree add /tmp/pr<n>
  pr-<n>`, build in a separate meson dir there, then
  `git worktree remove /tmp/pr<n> --force && git branch -D pr-<n>` to
  clean up.
- `fs_compile_project` is **broken for this repo currently** (tries
  `make -C build`, but the project is Meson+Ninja with no Makefile) — do
  not use it here; build with `ninja -C build` / `meson setup` via bash
  instead. `fs_run_tests` also doesn't fit this project's meson test
  layout — use `meson test -C build --print-errorlogs` via bash.
- Per-PR review reports are saved as `review-pr-<n>.md` (not the generic
  `review.md` from `review/review.md`'s template) since multiple PRs are
  being reviewed in the same session and would otherwise overwrite each
  other's report.
- `fs_read_lines` / `fs_file_codemap` / `fs_read_symbol` are strongly
  preferred over `view`/bash `cat` for this project: they add line
  numbers and an auto-appended symbol codemap (function name + start/end
  line), which is much more useful for locating code to edit than a bare
  file dump.
- User preference: prefer `EXIT_FAILURE`/`EXIT_SUCCESS` over bare
  `exit(1)`/`exit(0)`, and prefer `std::from_chars` (C++23) over
  `strtol`/`strtoul`/`atoi` for numeric CLI argument parsing.

# turbostar MCP tool fix update (2026-09-15)

- `fs_compile_project` was fixed (server working directory now correctly
  points at this project instead of `sandstone`) and is confirmed working:
  it runs `meson compile -C build` against the real `build` dir, correctly
  reports "no work to do" when clean and does real incremental
  recompiles/relinks when files change. Safe to use again for this repo.
- `run_cpp` was only *partially* fixed by the same working-directory
  change: compilation now correctly happens under
  `<project>/build/tmp_cpp/` and produces valid, runnable ELF binaries
  (verified manually with `file` + direct execution — they run fine), but
  the tool's own execution step still fails with exit 127 "No such file
  or directory" immediately after compiling, even though the binary is
  present on disk. Root cause looks like a separate bug (path
  resolution/timing) in the tool's run step, not the working-directory
  issue. Do not rely on `run_cpp` for this project yet; still use
  `turbostar-run_cpp`'s bash-equivalent (write a temp .cpp, `g++`, run
  manually) or plain bash g++ as a workaround if needed.

# RAPL stub fail-injection pattern for uninitialized-value regressions (2026-09-15)

- `tests/devices/stub_rapl_iface.cpp` now supports simulating "domain
  present but this one read call fails" via
  `rapl_fail_next_pp0_energy()` / `_dram_energy()` / `_pp1_energy()`.
  These set a one-shot static flag consumed by the corresponding
  `get_*_energy_status()`, which then returns -1 **without writing
  `*s`**, mirroring the real interface's failure semantics exactly
  (previously the stub could only succeed-from-queue or `assert()` on
  an exhausted queue — there was no way to test the "leaves output
  untouched on failure" contract that PR #223's bug was about).
- Pattern for writing this kind of regression test: reset the stub,
  push queued values for the constructor + `start_measurement()` reads,
  call `rapl_fail_next_pp1_energy()` right before the call under test
  (e.g. `end_measurement()`), then assert the device's computed power
  is deterministically `0.0` (works because the correct fix is
  `energy = last_energy` before the read) and that `last_energy` in
  `serialize()`/JSON output is unchanged from the last successful
  reading — i.e. the failed read must not corrupt state.
- To verify a test like this actually catches the bug (not just
  "happens to observe 0 by luck"): temporarily revert the fix, rebuild,
  and run the test binary under `valgrind --track-origins=yes` — an
  uninitialized-stack-double bug reliably produces both an assertion
  failure (garbage-derived non-zero power) AND valgrind
  "Uninitialised value" errors. Always restore the fix afterwards.
- Added a `valgrind` meson-suite entry (`valgrind-rapl-devices`) that
  runs `tests/devices/powertop-test-rapl-devices` under valgrind
  alongside the existing `valgrind-tui`/`valgrind-html`/etc. entries —
  this means future regressions of this class are caught automatically
  by `meson test --suite valgrind`, not just by luck of a deterministic
  assertion. Note: variables from `subdir('tests')` (e.g.
  `test_rapl_devices_exe`) ARE visible in the top-level `meson.build`
  after the `subdir()` call returns (Meson shares scope like a textual
  include), so wiring individual test executables into the top-level
  `valgrind` suite block is straightforward — no need to duplicate the
  `valgrind`/`valgrind_common_args` definitions inside subdirs.

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

# RAII migration progress log

Raw `new`/`delete` audit (2026-09-25): started at ~24 `delete` sites in
`src/`. Converted the single-owning-pointer cases (report_maker::formatter,
tune_window, newtab_window, cpu.cpp's perf_events, do_process.cpp's
perf_events) to `unique_ptr` — see commits fixing review item #1 and
"RAII pass 2". Remaining ~16 sites are all collection-ownership
(`vector<T*>`/`map<K,T*>`: past_results, all_devices, all_timers, all_work,
tab_windows, devlist's one/two/devpower) plus a handful of local
"create-or-discard" temporaries that feed those same raw-pointer vectors
(cpu_rapl_dev/dram_rapl_dev, i915-gpu's rapl_dev, runtime_pm's dev) —
converting those cleanly wants the container migration done first, so
they were deliberately left for a dedicated follow-up pass rather than
converted piecemeal.

**Re-verified 2026-09-28: no drift since the 2026-09-25 audit** — still
exactly 16 real `delete` statements / ~30 `new` allocations in `src/`
(excludes the stale `src/tuning/runtime.cpp~` backup and the vendored
`src/tuning/nl80211.h` kernel header, which only has `delete`/`new` in
comments). Since the 2026-09-25 pass, `power_meters`, `all_devfreq` +
`devfreq::dstates`, `perf_bundle::events`, `all_interrupts`,
`all_processes`, `all_proc_devices`, `all_tunables`/`all_untunables`, and
`abstract_cpu::children`/`cstates`/`pstates` have all separately been
converted to `vector<unique_ptr<T>>` (confirmed via grep — the `raii.md`
file itself no longer exists, so this log plus the ownership map above are
now the only source of truth).

**`all_devices` converted 2026-09-29** — `src/devices/device.h`/`.cpp`'s
`all_devices` is now `vector<unique_ptr<device>>`. All ~11 `create_*`
producers (ahci, alsa, backlight, i915-gpu, rfkill, runtime_pm,
thinkpad-fan/light, usb, network, xe-gpu) and `cpu.cpp::new_package()`
converted to `make_unique`/`push_back(std::move(...))`, with
create-or-discard temporaries (`cpu.cpp`'s `cpu_rapl_dev`/`dram_rapl_dev`,
`i915-gpu.cpp`'s `rapl_dev`, `runtime_pm.cpp`'s `dev`) now auto-deleted by
falling out of scope instead of explicit `delete`. All iteration sites
(`device_manager.cpp`, `processdevice.cpp`, `parameters.cpp`, `gpu-tab.cpp`)
changed `for (auto *d : ...)` → `for (auto &d : ...)`;
`network.cpp`'s `nics` map and `cpudevice`/`i915gpu`'s `child_devices`
were deliberately left as non-owning raw-pointer views. `clear_all_devices()`
is now just `all_devices.clear();`.

**Two gotchas hit during this conversion, worth knowing for the remaining
5 containers below:**
- *Stack-vs-heap test mocks*: `tests/base/test_processdevice.cpp` had
  `mock_device dev(...); all_devices.push_back(&dev);` (stack-allocated,
  fine for a raw-pointer vector since nothing ever deletes it). Once the
  vector owns via `unique_ptr`, its destructor calls `delete` on every
  element — pushing a stack address causes a crash/UB on teardown. Fixed
  by switching to `all_devices.push_back(std::make_unique<mock_device>(...));`.
  Audit every test file that pushes into a container being migrated for
  this exact pattern before flipping the container's type.
- *Key-function vtable link errors in stub-only test files*: several test
  files (`test_xe_core.cpp`, `test_parameters.cpp`,
  `test_learn_parameters.cpp`) declare their own local
  `all_devices` "stub" instead of linking the real `device.cpp`, because
  they never previously needed `device`'s implementation (a raw-pointer
  vector's destructor is trivial). `device`'s virtual destructor is
  inline in `device.h`, but `collect_json_fields()` — its Itanium ABI "key
  function" (first non-inline virtual) — is defined out-of-line in
  `device.cpp`, so the vtable is only emitted in that TU. Converting the
  stub to `vector<unique_ptr<device>>` makes the *global variable's own
  destructor* require `device`'s vtable (to `delete` through the base
  pointer), which fails to link with "undefined reference to vtable for
  device" unless `device.cpp` is part of that executable. Fix: add
  `../../src/devices/device.cpp` to the test's `meson.build` sources and
  delete the local stub definition (keep only the `extern` from
  `device.h`) rather than just retyping it. Check for this class of
  failure — a link error, not a compile error, so it only surfaces when
  actually building the test binaries — whenever an `extern`-only stub in
  a test file has its container type migrated to an owning smart pointer.

**`past_results` converted 2026-09-29** — `parameters.h`/`.cpp`'s
`past_results` is now `vector<unique_ptr<result_bundle>>`. `clone_results()`
now returns `unique_ptr<result_bundle>` (via `make_unique`) instead of a raw
pointer; both the ring-buffer overwrite in `store_results()` (parameters.cpp)
and `load_results()` (persistent.cpp) just do plain assignment/`std::move`
into the vector slot — the old element's `delete` is now automatic. Raw
`result_bundle *` is still used locally wherever a non-owning view is needed
(`.get()` at call sites like `compute_bundle()`, `get_result_value()`,
`bundle_power()`). No test-file link issues here (unlike `all_devices`) —
`result_bundle` is a plain struct with no virtual functions, so there's no
key-function/vtable gotcha; the test files that use `past_results` directly
(`test_parameters.cpp`, `test_learn_parameters.cpp`) already link the real
`parameters.cpp`, they just needed their own local `new`/`delete` push
patterns updated to `make_unique`/`std::move`.

**`all_timers` converted 2026-09-29** — `process/timer.cpp`'s `all_timers` is
now `map<uint64_t, unique_ptr<timer>>`. It's `static` to `timer.cpp` (not
`extern`'d via `timer.h`), so unlike the previous two containers, zero other
production or test files reference the map directly — the whole conversion
was contained to one file. `find_create_timer()` still *returns* a raw
`timer *` (non-owning observer, matches `do_process.cpp`'s usage pattern),
built via `make_unique` + `.get()` before `std::move`-ing into the map.
`all_timers_to_all_power()`'s structured-binding loop just needed
`all_power.push_back(t.get())` (`all_power` is `vector<power_consumer*>`,
a non-owning observer container, unchanged). `clear_timers()`'s manual
iterator-erase-with-`delete` loop collapsed to a plain `all_timers.clear();`.
No test-file gotcha this time (no key-function/vtable issue, no stack-mock
issue) — `tests/base/test_timer.cpp` only touches the map indirectly via
`find_create_timer()`/`clear_timers()`/`all_timers_to_all_power()`.

**`all_work` converted 2026-09-28** — `process/work.cpp`'s `all_work` is now
`map<unsigned long, unique_ptr<work>>`, same self-contained pattern as
`all_timers` (static to `work.cpp`, no test-file gotchas). One extra wrinkle
vs. `all_timers`: `all_work_to_all_power()` iterates via
`std::for_each(all_work.begin(), all_work.end(), add_work)` rather than a
structured-binding range-for, so the free function `add_work()`'s parameter
type had to change from `const std::pair<unsigned long, work*>&` to
`const std::pair<const unsigned long, unique_ptr<work>>&` (note: map's
`value_type` key is `const K`, so the `const` on the key type is required —
compiles either way with a range-for's `auto&`, but a raw `std::pair<...>&`
signature must match `map::value_type` exactly or the call silently fails to
bind and produces a "no matching function" error at the `for_each` call
site, not at `add_work`'s definition). `find_create_work()`/`clear_work()`
follow the same `make_unique`/`.get()`/`.clear()` pattern as `find_create_timer()`.

**Exact remaining raw-owning containers (2), still `vector<T*>`/`map<K,T*>`:**
1. `tab_windows` (`map<string, tab_window*>`, display.h/.cpp) — freed in
   `reset_display()`. **Scoped but NOT started 2026-09-28 — see notes below,
   harder than the previous 3 conversions, needs a deliberate session.**
2. `devlist.cpp`'s `one`/`two` (`vector<devuser*>`) and `devpower`
   (`vector<devpower*>`) — freed in `clean_open_devices()` /
   `collect_open_devices()`.

**`tab_windows` scoping notes (2026-09-28, conversion deferred):** unlike
`all_devices`/`past_results`/`all_timers`/`all_work`, this container has
**mixed ownership** today, not sole ownership — a straight retype will
either break things or paper over a real bug:
- Most tabs (Overview, Idle stats, Frequency stats, Device stats, Device
  Freq stats, the GPU tab) are created via `create_tab(name, translation)`
  with `w == nullptr`; `create_tab()` does `new tab_window` internally and
  `tab_windows` is the *sole* owner, freed by `reset_display()`'s
  `delete val` loop over every map entry.
- Two tabs — WakeUp (`wakeup/waketab.cpp`) and Tunables (`tuning/tuning.cpp`)
  — are different: each file already has its own `unique_ptr<subclass>`
  global (`newtab_window`, `tune_window`) that owns the object; they call
  `create_tab(name, translation, win.get(), ...)`, so `tab_windows[name]`
  only holds a **non-owning alias** to the same object. `shutdown_wakeup()`/
  `shutdown_tuning()` call `tab_windows.erase(name)` *before*
  `newtab_window.reset()`/`tune_window.reset()`, specifically to keep
  `reset_display()`'s later delete-loop from ever touching an
  already-separately-owned entry.
- **Pre-existing latent double-free bug found while scoping (not
  introduced by this migration, not yet fixed):** `reset_display()` is
  also called on fatal-error paths (`main.cpp`'s `out_of_memory()`,
  `perf/perf.cpp`, `cpu/intel_cpus.cpp`) that do *not* call
  `shutdown_tuning()`/`shutdown_wakeup()` first. If the WakeUp/Tunables
  tabs are active when one of those paths fires, `reset_display()`
  deletes the tab object once via the map, and then `newtab_window`/
  `tune_window`'s `unique_ptr` destructor (running at `exit()`'s static
  teardown) deletes it again. Masked in practice because these are
  abort/exit paths, but it's real UB.
- **Recommended fix when this is picked back up:** make `tab_windows`
  (`map<string, unique_ptr<tab_window>>`) the *sole* owner for every tab,
  including WakeUp/Tunables, and demote `tune_window`/`newtab_window` to
  plain non-owning `tab_window*` (or `wakeup_window*`/`tuning_window*`)
  observer pointers obtained via `.get()` after insertion — this also
  fixes the latent double-free as a side effect. Confirmed via grep that
  `tune_window`/`newtab_window` are referenced *only* inside
  `tuning.cpp`/`waketab.cpp` respectively (no external callers), so the
  blast radius is small: `display.h`/`.cpp`, `tuning/tuning.cpp`,
  `wakeup/waketab.cpp`, plus `.get()` touch-ups at every
  `tab_windows[name]` read site (`cpu.cpp`, `devfreq.cpp`,
  `device_manager.cpp`, `do_process.cpp`, `gpu-tab.cpp` — about a dozen
  call sites, all read-only lookups assigning to a local
  `class tab_window *`). No test file references `tab_windows` directly.
  Budget more time for this one than the previous three; it's a genuine
  ownership-model change, not a pure mechanical retype.

**Deliberately out of scope (documented exception, not a regular
container):** `perf_bundle::records` (`vector<void*>`) uses raw
`malloc`/`free` for event buffers (`perf_bundle.cpp`) — needs a custom
RAII wrapper rather than `unique_ptr<T>`, tracked as a separate follow-up,
not part of this vector-of-pointers migration.

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

# turbostar MCP tooling notes

- `turbostar-fs_run_tests` in this environment is configured with a stale
  build path (`/home/arjan/git/powertop/build`) that doesn't match this
  repo's actual checkout path (`/sdb1/arjan/git/powertop`), so it always
  fails with "No such build data file". Use `bash` + `meson test -C
  <build_dir>` / `ninja -C <build_dir>` directly instead for test runs in
  this session; there's no turbostar-native way to target a non-default
  build dir (e.g. `build_acov`) yet.
- `turbostar-fs_grep_files`'s `pattern` is a **literal string by default**;
  passing something like `"past_results|clone_results"` without
  `is_regex: true` searches for that exact literal (including the `|`),
  not an alternation — silently returns "No matches found" instead of an
  error. Always set `is_regex: true` when using regex metacharacters
  (`|`, `.*`, character classes, etc.), or split into separate calls.
- `turbostar-fs_replace_content` reports "Successfully replaced" even when
  `target_content` and `replacement_content` are identical (a true no-op)
  — it doesn't warn that nothing changed. Double-check edits that were
  meant to change something actually show up in a subsequent `view`/diff.

