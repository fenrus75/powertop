This repository is for PowerTOP, a linux tool to find the sources of power
consumption in a system.

Do **NOT** update this file to record the results of code review or fixes
for code review comments!

The code review rules for this project are in `review/review.md` and this
includes a style guide.

The class hiearchy is documented in `review/class.md` and this document
needs to be kept uptodate as changes to the class hierarchy are made.

# CPU frequency class split

`src/cpu/frequency.h` — extracted from `cpu.h`: contains `struct idle_state`,
`class frequency`, and the `LEVEL_C0`/`LEVEL_HEADER`/`PSTATE`/`CSTATE` macros.
Only depends on `lib.h` and standard headers. Safe to link in unit tests.

`src/cpu/frequency.cpp` — `frequency::frequency()` implementation (empty ctor).
Previously lived at the bottom of `cpu.cpp`.

`cpu.h` now `#include "frequency.h"` in place of the inline definitions.
`abstract_cpu` tests link `abstract_cpu.cpp` + `frequency.cpp` without the
heavy `cpu.cpp` chain.

# Base class serialize tests (tests/base/)

All 6 base classes have snapshot tests:
- `test_wakeup_base.cpp` — desc/score/enabled/toggle fields
- `test_tunable_base.cpp` — desc/score/result/toggle_good/toggle_bad
- `test_device_base.cpp` — class/name/hide/guilty/real_path
- `test_power_meter_base.cpp` — name/discharging/power/capacity
- `test_power_consumer_base.cpp` — all 8 member fields
- `test_abstract_cpu_base.cpp` — type/number/idle flag/empty arrays

Pattern for protected toggle fields: use a minimal `class test_X : public X`
subclass in the test file to expose protected members for initialization.

`powerconsumer_stubs.cpp` provides `all_parameters`, `get_parameter_value`
overloads to avoid linking `parameters.cpp` → `all_devices` chain.

`src/devices/device.cpp` — contains only the `class device` method
implementations (`device()`, `register_sysfs_path`, measurement virtuals,
`collect_json_fields`) plus the `all_devices` vector. Minimal includes;
safe to link in unit tests.

`src/devices/device_manager.cpp` — contains `create_all_devices`,
`clear_all_devices`, `devices_start/end_measurement`, `report_devices`,
`show_report_devices`. Has the heavy include chain (all device subtypes,
report/, display/, measurement/).

Unit tests link `device.cpp` directly. Only three symbols need stubs
(`global_power`, `save_all_results`, `register_devpower`) — placed in
`tests/devices/test_stubs.cpp`.

# Measurement source file split

`src/measurement/measurement.cpp` — contains only the `power_meter` base
class methods and the `power_meters` vector. Minimal includes; safe to
link in unit tests.

`src/measurement/measurement_manager.cpp` — contains all manager/detection
functions (`start/end_power_measurement`, `global_power`, `detect_power_meters`,
etc.) with the heavy include chain (acpi.h, extech.h, sysfs.h, opal-sensors.h,
parameters.h). Same pattern as device_manager.cpp.

Unit tests link `measurement.cpp` + `acpi.cpp` directly without needing stubs.

The process for working on this codebase always consists of 5 steps
1. Make the change
2. Build the project (with meson/ninja)
   - Use `ninja -C <builddir> -t clean && ninja -C <builddir>` when the build cache seems stale
   - The build output goes through a pipe in many invocations; "no work to do" after a clean means the build already completed in the same shell pipeline
3. Run the test suite: `ninja -C <builddir> test` (requires `-Denable-tests=true` at setup time)
   - When adding new tests, read `tests/testdesign.md` for conventions and patterns.
4. Code review the change to make sure it strictly matches the narrow objective
5. Git commit the change with a comprehensive git commit message (no need to
   ask permission)

Also read `review/tools.md` when you're asked to use the various
tooling to create and manipulate test data.

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

# pt_readlink() — mockable readlink wrapper

`pt_readlink(const std::string &path)` in `lib.cpp` wraps `readlink(2)`
(via `std::filesystem::read_symlink`) with test framework hooks:
- Record mode: stores the resolved target (or "" for failure)
- Replay mode: returns the stored value
- Normal mode: calls `std::filesystem::read_symlink()` directly

Record format: `L base64(target) path`
- base64 token first (no spaces) so path (which may contain spaces)
  is safely everything after the first space.
- Empty base64 (`L  path`) = readlink failed → returns "".

`devlist.cpp`, `rfkill.cpp`, and `network.cpp` all use `pt_readlink`
instead of raw `readlink` / `std::filesystem::read_symlink`.
`<filesystem>` removed from rfkill.cpp and network.cpp.

First test using L records: `tests/devices/test_rfkill_serialize.cpp`
(3 scenarios: no driver, device/driver, device/device/driver).

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

## rfkill: sysfs_path was missing (fixed)

The rfkill constructor did not set `sysfs_path`. Added:
  `sysfs_path = path;`
  `register_sysfs_path(path);`
at the top of the constructor (matching runtime_pmdevice pattern).
Note: `register_sysfs_path()` sets `real_path` only (walks /device/ chain
and calls realpath()); the derived-class `sysfs_path` field must be set
directly.

## Measurement cycle tests

rfkill, usbdevice, runtime_pmdevice follow the pattern:
  construct (may consume L+R records) → start_measurement() → end_measurement() → serialize()

Fixture file must have records in exact call order. For rfkill: 2 L records
(constructor) + 4 R records (2 for start, 2 for end). For usbdevice: 5 R
records (constructor) + 4 R records (start + end).

## process schedule/deschedule test

`schedule_thread(time, tid)` and `deschedule_thread(time, tid)` are pure
in-memory. Reuse existing fixture for construction, call `reset()` after
construction, then call schedule/deschedule without any fixture. Verify
`accumulated_runtime` via serialize JSON and `usage_summary()` directly.

`usage_summary()` formula: `accumulated_runtime / 1000000.0 / measurement_time / 10`

If you have questions or improvement suggestions for something the user
asked, or if you think the user made a mistake in the prompt: stop and **ask** the user!

## backlight: display_is_on() virtual hook

`dpms_screen_on()` is a static function that uses `opendir()` (not mocked)
to scan `/sys/class/drm/card0`, then calls `read_sysfs_string()` on found
entries (which IS intercepted by the test framework). This causes SIGABRT in
replay mode on machines with a real DRM card.

Fix: extracted `virtual int display_is_on()` as a protected method on
`backlight` that delegates to `dpms_screen_on()`. Tests subclass `backlight`
and override `display_is_on()` to return 1 (screen always on).

## trace_tool.py: --path filter for list command

`trace_tool.py list --path SUBSTR file.ptrecord` filters output to entries
whose path contains SUBSTR. Useful for inspecting large fixture files.
The `--content` / `-c` flag can be combined with `--path` / `-p`.

## Fixture creation with trace_tool.py add

Use `trace_tool.py add FILE R PATH VALUE` to build fixture files from
scratch (creates file if it doesn't exist). Values are plain strings,
auto-encoded to base64. Validate after building with `trace_tool.py validate`.

Example for backlight (2 reads in start_measurement):
  trace_tool.py add backlight_start.ptrecord R /sys/class/backlight/lcd/max_brightness 100
  trace_tool.py add backlight_start.ptrecord R /sys/class/backlight/lcd/actual_brightness 60
## trace_tool.py: T record support

`trace_tool.py add FILE T "sec usec"` now adds time snapshot records.
Used for `pt_gettime()` interception in classes that measure elapsed time
(e.g. devfreq). The `add` command choices list was updated to include `T`.

## Test infrastructure: stub files

When a class pulls in a heavy dependency chain through one function:
- Create `tests/devices/stub_display.cpp` for `create_tab` / `get_ncurses_win`
- Create `tests/base/stub_runtime_pm.cpp` for `device_has_runtime_pm`
Both stubs inline the actual logic using intercepted `read_sysfs()` /
`read_file_content()` — so the stubs remain correctly intercepted in tests.

## tuningusb / tuningi2c patterns

For any `tunable`-derived class: `serialize()` → `collect_json_fields()` →
`result_string()` → `good_bad()` → one extra sysfs read. Constructor fixture
must have N+1 records (N ctor reads + 1 for good_bad in serialize).

For toggle tests: construct the object with the N-record ctor fixture, then
`reset()` + load a 1-record toggle fixture for the `good_bad()` read inside
`toggle()`. The write is captured in `write_log`.

## devfreq: measurement cycle with T records

`devfreq` measurement cycle:
- `start_measurement`: T(before) + R(trans_stat with time_before values)
- `end_measurement`: R(trans_stat with time_after values) + T(after)
- `process_time_stamps()`: `time_after = 1000 * (end_ms - start_ms)`;
  last state (freq=0, Idle) gets residual = sample_time - active
- `fill_freq_utilization(idx)`: `100 * time_after / sample_time`

## cpudevice: no I/O

`cpudevice` constructor calls `get_param_index`/`get_result_index` (parameters
subsystem), but no I/O. All tests are fixture-free. Link:
`lib.cpp + test_framework.cpp + parameters.cpp + device.cpp +
measurement.cpp + cpudevice.cpp + test_stubs.cpp`.

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

Current baseline: **20.7% line / 32.3% function** (src/ only, after lib.cpp round 2).
lib.cpp: 74.8% lines (up from 72.7%, up from original 59%).

## lib.cpp coverage targets (achieved round 2)

- Fix `process_glob_no_match`: use existing tmpdir so GLOB_NOMATCH fires (502-505)
- `pt_readlink` fail+record path (268): non-existent path while in recording mode
- `align_string` invalid UTF-8 (311): setlocale("C.UTF-8") + \x80 lead byte
- `read_kallsyms` empty content (118): new binary `test_lib_kallsyms_empty` with N fixture

## lib.cpp future opportunity: libpci tests

Link a test WITHOUT `-DHAVE_NO_PCI` against `libpci` to cover `pci_id_to_name`
and `end_pci_access` real implementations (lines 336-354, ~19 lines → ~80%+).
See `/tmp/lib.md` for full design.

Tests added to cover previously uncovered paths:
- `format_watts` (lines 319-331): test_format_watts_normal/tiny_zero/aligned
- `fmt_prefix` UTF-8 detection block (382-387): test_fmt_prefix_utf8_detection_ascii/unicode
- `fmt_prefix` µ prefix (line 434): test_fmt_prefix_micro_utf8
- `fmt_prefix` omag==2 path (line 414): test_fmt_prefix_hundred
- `end_pci_access` stub (363-365): test_end_pci_access_stub
- `ui_notify_user_console` (617-620): test_ui_notify_user_console
- `pt_readlink` non-replay paths (263-276): test_pt_readlink_real_symlink/nonexistent/recording
- `process_glob` GLOB_NOMATCH (502-505): test_process_glob_no_match
- `read_sysfs` catch block (210-214): test_read_sysfs_non_integer_content
- `hz_to_human` MHz digits==2 (line 102): test_hz_to_human_mhz_two_digits
- `read_kallsyms` bad-input paths (126,131,134): updated replay fixture

## HTML report tests (tests/report/)

`report_formatter_html` and `report.cpp` are tested via:
- Full pipeline: replay fixture → `init_report_output()` + `finish_report_output()` → tidy validation
- Direct formatter tests: local `report_maker r(REPORT_HTML)` — avoids global state
- All `add_table()` position branches (T, L, TL, TC, TLC) need separate tables
  - TC/TLC require `title_mod > 0` (use `init_core_table_attr` / `init_cpu_table_attr`)
- Tidy detection: `find_program('tidy', required: false)` → `-DHAVE_TIDY -DTIDY_BIN="..."` 
- Tidy accepts exit 0 (ok) or 1 (warnings); fails on 2 (errors)
- `read_os_release()` was converted from `std::ifstream` to `read_file_content()` + 
  `std::istringstream` so it's interceptable. Pattern: same as `cpu_model()`.
- `css_h` custom_target must be in executable sources for `css.h` include path

Fixture `tests/report/data/report_sysinfo.ptrecord` seeded from real hardware:
  Gigabyte X299 AORUS Gaming 3 Pro-CF / Intel i9-7900X / Debian forky/sid

## Current test count: 33 tests passing (33 executables)

Easy+Medium candidates all done:
backlight, thinkpad_fan/light, work, ethernet_wakeup,
tuningusb, tuningi2c, devfreq, opal-sensors, cpudevice.
