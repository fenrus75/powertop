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

# Coverage baseline (as of commit 8906999)

Overall: 29.0% lines (2654/9140), 43.2% functions (418/967)

Well-covered files (>85%, essentially done):
- `process/powerconsumer.cpp` 98%
- `process/interrupt.cpp` 89%
- `report/*.cpp`, `lib.cpp`, `timer.cpp`, `measurement/sysfs.cpp` > 90%

Partially covered, more tests possible but require sysfs fixtures:
- `cpu/abstract_cpu.cpp` 43% — measurement_start/end, wiggle need sysfs
- `cpu/cpu_linux.cpp` 30% — parse_cstates/pstates_start/end need sysfs
- `process/process.cpp` 39%
- `devices/runtime_pm.cpp` 32%
- `tuning/runtime.cpp` 45%

Practically untestable without hardware (0% or near):
- `main.cpp`, `cpu.cpp`, `intel_cpus.cpp`, `do_process.cpp`,
  `calibrate.cpp`, `perf/`, `display.cpp`
  (devlist.cpp and rapl_interface.cpp now have tests — see below)

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

