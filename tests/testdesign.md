# PowerTOP Test Design Guide

This document captures the conventions, infrastructure, and patterns used in
the PowerTOP test suite so that future agents and contributors can add new
tests without re-discovering them from scratch.

---

## Directory layout

```
tests/
├── test_helper.h               minimal assert macros (no external deps)
├── testdesign.md               this file
└── <area>/
    ├── meson.build             builds the test executable for this area
    ├── test_<subject>.cpp      test cases + main()
    └── data/
        ├── sample.txt          reference input files
        └── *.ptrecord          pre-baked replay fixtures
```

Add one subdirectory per functional area (e.g. `lib/`, `parameters/`,
`devices/`). Each area produces its own standalone test executable linked only
against the source files it actually needs.

Also read `review/tools.md` for tool helpers for this task.

---

## Build system integration

The test suite is opt-in. Enable it at configure time:

```
meson setup builddir -Denable-tests=true
ninja -C builddir test
```

The `enable-tests` option is defined in `meson_options.txt`. The top-level
`meson.build` calls `subdir('tests')` when it is set.

### Per-area meson.build pattern

```meson
test_<area>_exe = executable(
  'powertop-test-<area>',
  [
    '../../src/lib.cpp',          # only the files this test actually needs
    '../../src/test_framework.cpp',
    'test_<subject>.cpp',
  ],
  include_directories: [
    include_directories('../..'),   # for config.h in the build root
    include_directories('../../src'),
    include_directories('..'),      # for test_helper.h
  ],
  cpp_args: [
    '-DENABLE_TEST_FRAMEWORK',     # always on for test binaries
    '-DHAVE_NO_PCI',               # avoid pci.h dependency unless needed
    '-DTEST_DATA_DIR="' + meson.current_source_dir() / 'data' + '"',
  ],
  dependencies: [ncursesw_dep],    # lib.cpp pulls in ncurses headers
)

test('<area>: <subject>', test_<area>_exe)
```

Key points:
- **Always define `-DENABLE_TEST_FRAMEWORK`** in test binaries — the recording
  and replay APIs are compiled out otherwise.
- **`-DHAVE_NO_PCI`** avoids a `libpci` link dependency unless the test
  subject genuinely needs PCI lookups.
- **`-DTEST_DATA_DIR`** is set to the absolute source-tree path of the `data/`
  subdirectory at configure time, so the test binary can locate fixtures
  regardless of where the build directory is.
- Variables from the top-level `meson.build` (e.g. `ncursesw_dep`,
  `libtracefs_dep`) are in scope in all `subdir()` files — use them directly.

---

## Test helper macros (`tests/test_helper.h`)

| Macro | Purpose |
|-------|---------|
| `PT_ASSERT_EQ(a, b)` | fail if `a != b`; prints both values on failure |
| `PT_ASSERT_TRUE(expr)` | fail if `expr` is false |
| `PT_FAIL(msg)` | unconditional failure with a message |
| `PT_RUN_TEST(fn)` | call `fn()`, print its name before/after |
| `pt_test_summary()` | print pass/fail counts; return 1 if any failed |

Each test file defines its own `main()` that calls `PT_RUN_TEST` for each case
and returns `pt_test_summary()`. Meson's `test()` uses the exit code to
determine pass/fail.

---

## The test_framework_manager singleton

`src/test_framework.h` / `src/test_framework.cpp` implement a record/replay
system for all I/O that powertop performs. Three modes:

| Mode | What happens |
|------|-------------|
| **normal** | real file reads, real MSR reads, real wall clock |
| **recording** | real I/O, but every read/write is serialised to a `.ptrecord` file |
| **replay** | no real I/O; all reads/writes are served from the `.ptrecord` file |

The manager is a **singleton** (`test_framework_manager::get()`). Between test
cases always call **`reset()`** to clear all recorded/replayed state:

```cpp
test_framework_manager::get().reset();
```

The `reset()` method sets `recording = false` and `replaying = false`, clears
all queues, and clears the filename fields. The destructor checks
`if (recording)` before auto-saving, so after a `reset()` the destructor is
safe to run.

### Recording test pattern

```cpp
char tmpfile[] = "/tmp/pt_test_XXXXXX";
int fd = mkstemp(tmpfile);
close(fd);

test_framework_manager::get().set_record(tmpfile);
/* ... call the function under test ... */
test_framework_manager::get().save();    // explicit save before switching mode
test_framework_manager::get().reset();

test_framework_manager::get().set_replay(tmpfile);
/* ... call again and verify replayed output ... */
test_framework_manager::get().reset();
unlink(tmpfile);
```

`save()` is public and must be called explicitly when you want to persist the
recording mid-test (the destructor would also call it, but the singleton
outlives any one test function).

### Replay-from-fixture pattern

Pre-bake a `.ptrecord` file in `data/` and load it in the test:

```cpp
test_framework_manager::get().set_replay(DATA_DIR + "/my_fixture.ptrecord");
std::string result = read_file_content("/some/path");
PT_ASSERT_EQ(result, "expected content\n");
test_framework_manager::get().reset();
```

---

## `.ptrecord` file format

Each line represents one recorded event:

| Prefix | Meaning | Format |
|--------|---------|--------|
| `R` | successful file read | `R <path> <base64_content>` |
| `N` | file-not-found | `N <path>` |
| `W` | sysfs write | `W <path> <base64_content>` |
| `M` | MSR read | `M <cpu_decimal> <offset_hex> <value_hex>` |
| `T` | timestamp | `T <tv_sec> <tv_usec>` |

Events are replayed in the order they appear in the file, per-path for reads
and writes (each path has its own FIFO queue), and globally for timestamps.

To generate the base64 for a fixture file:

```bash
printf 'file content here\n' | base64 --wrap=0
```

Example fixture for a successful read at `/test/path`:

```
R /test/path SGVsbG8gd29ybGQK
```

Example fixture for a not-found at `/test/missing`:

```
N /test/missing
```

---

## Link stubs needed when compiling lib.cpp

`lib.cpp` declares `ui_notify_user` as an extern function pointer (defined in
`main.cpp`). Test binaries that compile `lib.cpp` but not `main.cpp` must
provide a stub definition:

```cpp
void (*ui_notify_user)(const std::string &) = nullptr;
```

Put this at the top of the test `.cpp` file (outside any function).

---

## What each powertop I/O function does in each mode

| Function | normal | recording | replay |
|----------|--------|-----------|--------|
| `read_file_content(path)` | reads disk | reads disk + records | returns from queue |
| `write_sysfs(path, val)` | writes disk | writes disk + records | verifies against queue |
| `read_msr(cpu, offset, val)` | reads MSR device | reads device + records | returns from queue |
| `pt_gettime()` | `gettimeofday()` | `gettimeofday()` + records | returns from queue |

---

## Conventions

- One `.cpp` file per subject function or tightly related group of functions.
- Call `test_framework_manager::get().reset()` at the **start** of every test
  function, not just between tests, so a crash in a prior test cannot
  contaminate the next one.
- Use `mkstemp` for temporary record files; always `unlink` them at the end of
  the test function.
- Data files in `data/` are read-only fixtures committed to the repo; never
  write to them from tests.
- Test binary names follow `powertop-test-<area>` (e.g.
  `powertop-test-lib`).
- Meson `test()` labels follow `'<area>: <subject>'` (e.g.
  `'lib: read_file_content'`).

---

## Known gotchas

### `kernel_function`: static cache is never reset

`kernel_function()` uses an internal `static int kallsyms_read` flag.  The
first call reads `/proc/kallsyms` (via `read_file_content`) and sets the flag
to 1.  Subsequent calls use the cached `kallsyms` map and never re-read.

`test_framework_manager::reset()` does **not** clear this flag or the map.
Consequences for test design:

- The `kernel_function` tests must be the **first** tests to run in their
  binary (so the replay fixture is consumed on the first real read).
- Put them in a dedicated binary or as the very first `PT_RUN_TEST` in `main()`.
- Only one complete test scenario (fixture load → lookup → unknown-address) can
  be exercised per binary run.

### `callback` type does not accept capturing lambdas

`process_directory` and `process_glob` take `typedef void (*callback)(const std::string&)`.
A C++ lambda with captures cannot be implicitly converted to a plain function
pointer.  Use a file-scope free function plus a file-scope `std::vector` to
collect results:

```cpp
static std::vector<std::string> results;
static void collect(const std::string& s) { results.push_back(s); }

// in test:
results.clear();
process_directory("/some/path", collect);
```

### Seeding fixtures from the live system

For replay fixtures that reflect real hardware state (sysfs values, MSR
readings), read the live values and base64-encode them rather than inventing
values.  This keeps fixtures meaningful and catches regressions when the
platform changes.

```bash
# sysfs integer
printf "$(cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq)\n" | base64 --wrap=0

# MSR (requires root or setuid binary)
# hand-craft from a known-good value if root is unavailable
```


POSIX specifies that `mbsrtowcs()` **ignores `nwc` when `dst` is a null
pointer**. `align_string` passes `nullptr` as `dst`, so the `max_sz` parameter
is silently ignored and the full string width is always counted. Tests that
assume `max_sz` limits the counted width will fail — the string is only padded
when its actual character count is less than `min_sz`.

---

## Serialize-based snapshot testing

Every PowerTOP object now has a `serialize()` method that returns a complete
JSON snapshot of all fields, including internal measurement state. This enables
a powerful **snapshot test pattern**: feed an object controlled input, call
`serialize()`, and compare the result against a known-good JSON string.

### Why this is useful

- No need to assert on every field individually — one string comparison covers
  all fields at once.
- The JSON output is human-readable and easy to inspect when a test fails.
- The expected string acts as a specification: a future change that silently
  alters internal state will cause the JSON to differ and the test to fail.
- Snapshot strings can be generated by running the object once under the
  record framework and printing `serialize()`, then committing that output as
  the expected fixture.

### Basic pattern

```cpp
// 1. Instantiate the object
MyDevice dev("/sys/path", "dev-name");

// 2. Feed it controlled sysfs data via the replay framework
test_framework_manager::get().reset();
test_framework_manager::get().set_replay(
    std::string(TEST_DATA_DIR) + "/my_device.ptrecord");

// 3. Drive the object through a measurement cycle
dev.start_measurement();
dev.end_measurement();
test_framework_manager::get().reset();

// 4. Snapshot the full object state as JSON and compare
std::string got = dev.serialize();
PT_ASSERT_EQ(got, expected_json);
```

### Generating the expected JSON string

Run the test once with the recording framework active and print the result:

```cpp
test_framework_manager::get().set_record("/tmp/seed.ptrecord");
dev.start_measurement();
dev.end_measurement();
test_framework_manager::get().save();
test_framework_manager::get().reset();
std::cout << dev.serialize() << "\n";   // copy-paste this as expected_json
```

Then commit `/tmp/seed.ptrecord` as `tests/<area>/data/<fixture>.ptrecord`
and hard-code the printed string as `expected_json` in the test.

### Testing object hierarchies

Because `serialize()` recurses into child objects (via `JSON_ARRAY`), you can
snapshot a full hierarchy with one call. For example, snapshotting an
`abstract_cpu` will include all its `idle_state`, `frequency`, and child CPU
objects. This makes it straightforward to test that a full CPU topology
initialises correctly from a replayed sysfs tree.

### Subset / field-presence checks

When an exact full-string match is too brittle (e.g. the object contains
timestamps or platform-varying values), check for the presence of specific
substrings instead:

```cpp
std::string got = dev.serialize();
PT_ASSERT_TRUE(got.find("\"name\":\"eth0\"") != std::string::npos);
PT_ASSERT_TRUE(got.find("\"start_pkts\":") != std::string::npos);
```

### JSON structure conventions

- Every serialized object is a JSON object `{ … }`.
- The last field is always `"_end":0` (sentinel added by `JSON_END()`) — do not
  assert on this field; treat it as an implementation detail.
- Arrays of child objects use `pt_json_array<T>`, which calls `serialize()` on
  each element: `[{…},{…}]`.
- `struct timeval` fields are split into two integer fields:
  `"fieldname_sec"` and `"fieldname_usec"`.

### Good starting candidates for snapshot tests

| Class | Why it's simple |
|-------|-----------------|
| `alsa` | two sysfs counter reads, fixed paths, no children |
| `backlight` | reads min/max/current level from sysfs, no children |
| `rfkill` | reads two soft/hard block integers, no children |
| `runtime_pmdevice` | reads suspended/active time counters from sysfs |
| `process` | fields set directly from `/proc` reads, no sysfs, no children |

---

## Coverage-driven test development

Measuring line and function coverage before and after adding a test makes
it easy to verify that new tests actually exercise the intended code paths
and to track overall test-suite progress.

### One-time setup

```bash
meson setup build_cov -Db_coverage=true -Denable-tests=true
```

This creates a separate build directory instrumented with gcov.  Keep it
alongside the normal `build/` directory — it does not affect the regular
build.

> **Why not `ninja coverage`?**  The built-in Meson coverage target runs
> lcov but fails with a "duplicate function symbol" error because
> `test_framework.cpp` is compiled into every test binary *and* the main
> powertop binary.  The helper script below passes
> `--ignore-errors inconsistent` to work around this.

### Capturing a coverage snapshot

`scripts/coverage_report.sh` automates the full lcov pipeline:

```bash
# Build + run tests (always do this first to populate .gcda files)
ninja -C build_cov test

# Capture a named snapshot
scripts/coverage_report.sh <label> [build_cov]
```

The script writes three files to `/tmp/`:

| File | Contents |
|------|----------|
| `/tmp/pt_<label>_src.info` | Filtered tracefile (src/ files only) |
| `/tmp/pt_<label>_html/` | HTML report (open `index.html` in a browser) |

It also prints a per-file summary table to stdout.

### Before / after workflow

This is the recommended process every time you add a new test:

```bash
# Step 1 — run tests to get current coverage
ninja -C build_cov test

# Step 2 — record baseline
scripts/coverage_report.sh before
```

The last line of the output shows the overall totals, e.g.:
```
Total:  |15.6%  8969|25.2%  955|
```

```bash
# Step 3 — add your new test and fixture, edit meson.build, then rebuild
ninja -C build_cov test

# Step 4 — record new snapshot
scripts/coverage_report.sh after

# Step 5 — compare (the totals lines are enough for a quick check)
```

Because the summary is printed to stdout you can also capture it to a file
for a diff:

```bash
scripts/coverage_report.sh before 2>&1 | tee /tmp/before.txt
# ... add test ...
ninja -C build_cov test
scripts/coverage_report.sh after  2>&1 | tee /tmp/after.txt
diff /tmp/before.txt /tmp/after.txt
```

### Targeting a specific file

To see line-by-line coverage for a single source file, open the HTML report
and navigate to it, or use `lcov --list` with a grep:

```bash
lcov --list /tmp/pt_after_src.info | grep rfkill
```

### Current baselines (as of last PR)

| Metric | Value |
|--------|-------|
| Line coverage | 15.6% (1 400 / 8 969 lines) |
| Function coverage | 25.2% (241 / 955 functions) |

Files with the most room to improve (sorted by current line coverage):

| File | Lines covered |
|------|--------------|
| `devices/network.cpp` | 0% |
| `devices/ahci.cpp` | 0% |
| `cpu/cpu.cpp` | 0% |
| `process/interrupt.cpp` | 41% |
| `tuning/tuningsysfs.cpp` | 64% |
| `devices/usb.cpp` | 65% |
| `devices/alsa.cpp` | 55% |

---

## Python helper scripts for pass/fail analysis

For tests whose pass/fail logic is too complex or verbose to express cleanly
in C++ (e.g. deep JSON comparison, numeric tolerance checks, diff-style
reporting), write a small Python helper script and invoke it from the C++ test
binary via `system()` or `popen()`, or register it directly as a Meson `test()`
target.

### When to use a Python helper

- **Structured JSON comparison**: comparing two `serialize()` outputs while
  ignoring specific keys (e.g. timestamp fields, platform-varying values) is
  much cleaner in Python than in C++ string manipulation.
- **Numeric tolerance**: asserting that a floating-point field is within ±5 %
  of an expected value without adding a floating-point epsilon framework to the
  C++ tests.
- **Diff reporting**: when a snapshot mismatch occurs, a Python script can print
  a human-readable field-by-field diff (e.g. using `jsondiff` or the stdlib
  `json` + `difflib` modules) instead of printing two very long strings.
- **Cross-run regression checks**: comparing `serialize()` output captured
  across two different builds or two runs on the same hardware.

### Registering a Python test in Meson

```meson
# A pure Python analysis script registered as a Meson test target:
test('devices: alsa snapshot',
  find_program('python3'),
  args: [files('check_alsa_snapshot.py'),
         meson.current_source_dir() / 'data' / 'alsa_expected.json'],
  workdir: meson.current_source_dir(),
)
```

The script exits 0 for pass, non-zero for fail — Meson uses the exit code.

### Pattern: C++ binary dumps JSON, Python validates

```cpp
// In the C++ test binary — write serialize() output to stdout:
std::cout << dev.serialize() << "\n";
```

```python
# check_alsa_snapshot.py — read from a file or pipe, parse, validate
import sys, json

with open(sys.argv[1]) as f:
    expected = json.load(f)

actual = json.loads(sys.stdin.read())   # or load from a fixed output file

def check(key, exp_val):
    if actual.get(key) != exp_val:
        print(f"FAIL: {key}: expected {exp_val!r}, got {actual.get(key)!r}")
        sys.exit(1)

check("name", "hw:0,0")
check("start_active", 0)
print("PASS")
```

### Ignoring volatile fields

```python
IGNORE_KEYS = {"_end", "stamp_before_sec", "stamp_before_usec",
               "stamp_after_sec",  "stamp_after_usec"}

def json_eq_ignore(a, b):
    ka = {k: v for k, v in a.items() if k not in IGNORE_KEYS}
    kb = {k: v for k, v in b.items() if k not in IGNORE_KEYS}
    return ka == kb
```

### Script location convention

Place helper scripts alongside the test they support:

```
tests/<area>/
├── meson.build
├── test_<subject>.cpp
├── check_<subject>.py        ← Python validator
└── data/
    ├── <fixture>.ptrecord
    └── <fixture>_expected.json
```
