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
