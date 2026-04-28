# PowerTOP Medium Severity Review Findings

## Review of calibrate.cpp, calibrate.h, lib.cpp, devlist.cpp, main.cpp — batch 01

### Medium item #1 : `using namespace std;` in calibrate.cpp

Location: src/calibrate/calibrate.cpp : 48

Description:
`using namespace std;` is present at file scope. The project style guide (style.md §4.2) explicitly prohibits this; all standard-library elements must be referenced via the `std::` prefix. Code earlier in the same file already uses `std::string` and `std::format` correctly, making the `using` directive redundant and inconsistent.

Severity rationale: Namespace pollution can cause silent name collisions with POSIX and project symbols; it is a banned construct per the project coding rules.

Suggested fix: Remove `using namespace std;` and qualify any bare `string`, `map`, `vector`, `ifstream`, etc. with `std::`.

---

### Medium item #2 : `using namespace std;` in lib.cpp

Location: src/lib.cpp : 109

Description:
`using namespace std;` appears after a block of code that already uses explicit `std::` prefixes, and before code that mixes both styles. This is prohibited by style.md §4.2 and general-c++.md.

Severity rationale: Same as Medium item #1 — namespace pollution and banned construct.

Suggested fix: Remove the directive; qualify all standard-library identifiers with `std::`.

---

### Medium item #3 : `using namespace std;` in devlist.cpp

Location: src/devlist.cpp : 43

Description:
`using namespace std;` at file scope, prohibited by the project style guide.

Severity rationale: Same as Medium items #1 and #2.

Suggested fix: Remove and add `std::` qualifiers throughout.

---

### Medium item #4 : Error message in `burn_disk` not translatable

Location: src/calibrate/calibrate.cpp : 232

Description:
```cpp
printf("Error: %s\n", strerror(errno));
```
The string literal `"Error: %s\n"` is not wrapped in `_()`. All user-visible strings must be internationalised (rules.md, style.md §5). The message is also emitted to `stdout` instead of `stderr`.

Severity rationale: Violates the project i18n requirement; non-English users will always see the untranslated string.

Suggested fix:
```cpp
fprintf(stderr, _("Error writing calibration file: %s\n"), strerror(errno));
```

---

### Medium item #5 : `pthread_create` return value not checked in calibration functions

Location: src/calibrate/calibrate.cpp : 248, 264, 369

Description:
`cpu_calibration`, `wakeup_calibration`, and `disk_calibration` all call `pthread_create` without checking the return value. If thread creation fails, `stop_measurement` is still set to 0 and `one_measurement` is called, but there is no thread to drive CPU/wakeup/disk load. Calibration data collected in that phase will be silently invalid.

Severity rationale: Silent calibration failure leads to incorrect power model parameters being saved to `saved_parameters.powertop`.

Suggested fix: Check the return value of `pthread_create`; log an error and return early if it fails.

---

### Medium item #6 : `burn_cpu_wakeups` — undefined behaviour casting pointer to `long`

Location: src/calibrate/calibrate.cpp : 210, 264

Description:
```cpp
tm.tv_nsec = (unsigned long)dummy;   // dummy is void *
...
pthread_create(&thr, NULL, burn_cpu_wakeups, (void *)interval);
```
Converting a pointer to `unsigned long` and then storing it in `tv_nsec` (which is of type `long`) is a round-trip pointer → integer → integer that is undefined behaviour in C++. Although it works on most 64-bit Linux targets, it is technically UB and violates general-c++.md ("Undefined behavior language constructs are a violation of coding style").

Severity rationale: Technically undefined behaviour; a cleaner approach avoids the risk of miscompilation under aggressive optimisers.

Suggested fix: Pass the interval via a heap-allocated struct or a file-scope variable rather than through the thread argument pointer.

---

### Medium item #7 : `one_measurement` — workload thread creation error not translatable

Location: src/main.cpp : 239

Description:
```cpp
fprintf(stderr, "ERROR: workload measurement thread creation failed\n");
```
The string literal is not wrapped in `_()`, violating the i18n requirement.

Severity rationale: Violates mandatory i18n rule for all user-visible strings.

Suggested fix:
```cpp
fprintf(stderr, _("ERROR: workload measurement thread creation failed\n"));
```

---

### Medium item #8 : Debug message not translatable

Location: src/main.cpp : 545

Description:
```cpp
printf("Learning debugging enabled\n");
```
User-visible string not wrapped in `_()`.

Severity rationale: Violates i18n requirement.

Suggested fix:
```cpp
printf(_("Learning debugging enabled\n"));
```

---

### Medium item #9 : Case `'c'` (calibrate) does not exit after calibration

Location: src/main.cpp : 471–474

Description:
```cpp
case 'c':
    powertop_init(0);
    calibrate();
    break;
```
After the `break`, the `getopt` loop ends and `powertop_init(auto_tune)` is called again (line 539), followed by a potential interactive display mode or report generation. There is no `exit()` after `calibrate()`. If a user runs `powertop --calibrate`, the tool silently starts the interactive or report mode immediately after calibration completes, which is likely unintended.

Severity rationale: Incorrect program flow; calibration is a standalone operation and should terminate the process.

Suggested fix: Add `exit(0);` after `calibrate();`.

---

### Medium item #10 : `load_parameters` called twice in `powertop_init`

Location: src/main.cpp : 407, 422

Description:
`load_parameters("saved_parameters.powertop")` is called at line 407 (before parameter registration) and again at line 422 (after). The second call overrides the default registered parameters with saved values, which may be the intent; but calling it before registration at line 407 loads values that are then immediately overwritten by `register_parameter` calls, making the first call a no-op. At minimum this is wasteful; it also suggests a latent ordering bug.

Severity rationale: Redundant call with unclear intent; first call is ineffective and may mask a future ordering bug.

Suggested fix: Remove the first `load_parameters` call (line 407), keeping only the one after all `register_parameter` calls.

## Review of src/test_framework.h, src/test_framework.cpp, src/display.cpp, src/display.h, src/lib.h — batch 02

### Medium item #1 : `using namespace std;` in test_framework.cpp

Location: src/test_framework.cpp : 18
Description: `using namespace std;` is present at file scope in the .cpp implementation file. While less dangerous than in a header, it still violates style rule 4.2 and general-c++.md ("deprecated construct"). All subsequent unqualified STL identifiers (`string`, `vector`, `ofstream`, `getline`, etc.) rely on this directive.
Severity rationale: Style/rules violation; medium because it only affects this translation unit.
Suggested fix: Remove `using namespace std;` and prefix all STL types with `std::`.

---

### Medium item #2 : `using namespace std;` in display.cpp

Location: src/display.cpp : 36
Description: Same issue as medium item #1 — `using namespace std;` at file scope in a .cpp file. All bare `string`, `vector`, and `map` references in this file rely on it.
Severity rationale: Style/rules violation; medium because it only affects this translation unit.
Suggested fix: Remove `using namespace std;` and use `std::` prefix throughout.

---

### Medium item #3 : `#ifndef`/`#define` guard instead of `#pragma once` in test_framework.h

Location: src/test_framework.h : 11
Description: The file uses an old-style `#ifndef _INCLUDE_GUARD_TEST_FRAMEWORK_H_` / `#define` / `#endif` guard. Style rule 4.1 mandates `#pragma once`.
Severity rationale: Style violation; also the guard name uses a reserved double-underscore prefix (`_INCLUDE_GUARD_…_H_`), which is UB in C++ (names starting with `_` followed by upper-case are reserved).
Suggested fix: Replace lines 11–12 and 104 with a single `#pragma once`.

---

### Medium item #4 : `#ifndef` guard instead of `#pragma once` in display.h

Location: src/display.h : 25
Description: Same issue — old `#ifndef __INCLUDE_GUARD_DISPLAY_H_` guard. The guard name also uses the reserved `__` prefix.
Severity rationale: Style violation + reserved identifier usage.
Suggested fix: Replace with `#pragma once`.

---

### Medium item #5 : `#ifndef` guard instead of `#pragma once` in lib.h

Location: src/lib.h : 25
Description: `#ifndef INCLUDE_GUARD_LIB_H` guard. While the guard name itself is not reserved here, `#pragma once` is the project standard.
Severity rationale: Style violation.
Suggested fix: Replace with `#pragma once`.

---

### Medium item #6 : Use of deprecated `resetterm()` in `reset_display()`

Location: src/display.cpp : 87
Description: `resetterm()` is an obsolete ncurses function (removed in newer ncurses versions). The correct modern API is `endwin()`, which restores the terminal state after curses use. Using `resetterm()` may fail to link against current ncurses or produce wrong terminal behaviour.
Severity rationale: Deprecated/removed API that can break on modern systems.
Suggested fix:
```cpp
endwin();
```

---

### Medium item #7 : Hardcoded 120-column width in `show_tab()`

Location: src/display.cpp : 118, 123
Description: `mvwprintw(tab_bar, 0,0, "%120s", "")` and the same pattern for `bottom_line` pad the line to exactly 120 characters regardless of the actual terminal width (`COLS`). On narrower terminals this overflows the window; on wider terminals the bar is not filled. This is also a printf format-string that doesn't respect COLS.
Severity rationale: Functional display bug on any terminal not exactly 120 columns wide.
Suggested fix: Use `COLS` dynamically:
```cpp
mvwprintw(tab_bar, 0, 0, "%*s", COLS, "");
```

---

### Medium item #8 : Hardcoded tab-name string comparisons in `cursor_down()`

Location: src/display.cpp : 247
Description: `tab_names[current_tab] == "Tunables" || tab_names[current_tab] == "WakeUp"` hardcodes internal tab key names in the cursor navigation logic. This breaks if the tab name is ever changed and creates a fragile coupling between display.cpp and any code that creates those tabs. `init_display()` itself does not create "Tunables" or "WakeUp" tabs, so this comparison is already stale relative to the tabs initialised in this file.
Severity rationale: Fragile design; logic depends on magic string literals scattered in separate code.
Suggested fix: Introduce a flag or virtual method on `tab_window` to indicate that the tab uses cursor-tracking, rather than comparing by name.

---

### Medium item #9 : User-visible error strings in test_framework.cpp not wrapped in `_()`

Location: src/test_framework.cpp : 75, 81–84, 120, 144
Description: Strings such as `"TEST FAIL: Unexpected write to: "`, `"Failed to open record file: "`, and `"Failed to open replay file: "` are printed to `stderr` where they are visible to users running the tool. Per rules.md and style.md §5, all user-visible strings must be wrapped in `_()` for gettext internationalisation.
Severity rationale: i18n rule violation; strings will not be translatable.
Suggested fix: Wrap each string literal: e.g. `cerr << _("Failed to open record file: ") << ...`.

---

### Medium item #10 : C-style `typedef` for callback type in lib.h

Location: src/lib.h : 102
Description: `typedef void (*callback)(const std::string&);` uses the C-style `typedef` syntax. In C++11 and later (the project uses C++20) the preferred form is a `using` alias.
Severity rationale: Style / C++ best-practice violation.
Suggested fix:
```cpp
using callback = void (*)(const std::string&);
```

---

### Medium item #11 : `newpad(1000, 1000)` with hardcoded magic numbers in `create_tab()`

Location: src/display.cpp : 51
Description: Each tab's pad is created with a fixed 1000×1000 cell size. This magic number is repeated in the scroll-limit guards in `cursor_down()` and `cursor_right()` (lines 246, 301) without any shared constant. Using the actual content size or a named constant would make the intent clear and prevent the limit checks from getting out of sync if the pad size is ever changed.
Severity rationale: Magic number; creates a latent inconsistency between pad allocation and scroll boundary guards.
Suggested fix: Define `constexpr int PAD_MAX = 1000;` and use it in `newpad` and all guard comparisons.

## Review of src/tuning/tunable.h, src/tuning/tunable.cpp, src/tuning/runtime.h, src/tuning/runtime.cpp — batch 03

### Medium item #1 : Duplicate and malformed `<unistd.h>` include in runtime.cpp

Location: src/tuning/runtime.cpp : 28, 34
Description: `unistd.h` is included twice. The first occurrence at line 28 uses quotes (`#include "unistd.h"`), which is incorrect for a system header — it causes the compiler to search project directories first. The second occurrence at line 34 uses the correct angle-bracket form (`#include <unistd.h>`). The first include should be removed.
Severity rationale: Using quotes for system headers can cause subtle include-path issues and is a misuse of the include syntax. Having duplicate includes is also wasteful.
Suggested fix: Remove line 28 (`#include "unistd.h"`), keep line 34 (`#include <unistd.h>`).

### Medium item #2 : Missing `override` specifier on virtual methods in `runtime_tunable`

Location: src/tuning/runtime.h : 39–41
Description: `good_bad()` and `toggle()` override virtual methods from `tunable` but do not use the `override` keyword. Without `override`, a typo in the method signature would silently create a new virtual function instead of overriding the base class method.
Severity rationale: Missing `override` is a C++11+ best-practice violation. It prevents the compiler from catching signature mismatches.
Suggested fix:
```cpp
virtual int good_bad(void) override;
virtual void toggle(void) override;
```

### Medium item #3 : C header `<limits.h>` used instead of C++ header `<climits>`

Location: src/tuning/runtime.h : 29
Description: The C header `<limits.h>` is included. In C++ code, the corresponding C++ header `<climits>` should be used, which places all declarations in the `std` namespace.
Severity rationale: Using C headers in C++ code is a style violation per general-c++.md (C++20 standard compliance).
Suggested fix: Replace `#include <limits.h>` with `#include <climits>`. Also applies to runtime.cpp line 37.

### Medium item #4 : C header `<string.h>` used instead of C++ header `<cstring>`

Location: src/tuning/tunable.cpp : 28 ; src/tuning/runtime.cpp : 30
Description: Both files include `<string.h>` (C header) instead of `<cstring>` (C++ header). Additionally, given that these files use `std::string` throughout and do not appear to call any `string.h` functions, this include may be unnecessary entirely.
Severity rationale: Same as Medium #3 — C-style headers in C++ code.
Suggested fix: Replace `#include <string.h>` with `#include <cstring>` in both files, or remove if unused.

### Medium item #5 : Raw owning pointers in `all_tunables` / `all_untunables` vectors

Location: src/tuning/tunable.h : 89–90 ; src/tuning/runtime.cpp : 151–156, 169–171, 187–190
Description: `all_tunables` and `all_untunables` hold `class tunable *` raw owning pointers. Objects allocated with `new` in `add_runtime_tunables()` are pushed into these vectors and are never deleted. This is a memory leak. The same pattern was noted for other vectors in batch 02.
Severity rationale: Repeated memory allocation without deallocation; affects every device enumeration run. Using raw owning pointers is a C++ best-practice violation.
Suggested fix: Change the vector types to `std::vector<std::unique_ptr<tunable>>` and update all push_back calls and consumers to use `std::move` / `.get()` as appropriate.

### Medium item #6 : Block-device scan logic runs only once; devices associated with arbitrary PCI entry

Location: src/tuning/runtime.cpp : 127, 173–192
Description: The `count` variable (initialized to 0, set to 1 after the first outer-while iteration) causes the block-device `for (char blk...)` loop to execute only on the first PCI device encountered in the directory. Subsequent PCI device iterations skip it entirely. Furthermore, the `runtime_tunable` for each disk is constructed with `entry->d_name` (the current PCI device's name) as the `dev` argument, which is the PCI device at an arbitrary iteration order — not necessarily the one the disk is attached to. The `count` variable name gives no indication of its intended purpose.
Severity rationale: Functional correctness issue and confusing code. Disk tunables may be silently skipped or associated with the wrong PCI device.
Suggested fix: If the intent is to scan block devices only once (since their paths are global, not per-PCI-device), move the block-device loop outside the `while` directory-iteration loop entirely. Replace `count` with a clearly named boolean such as `bool block_devices_scanned = false`.

### Medium item #7 : Unused includes in runtime.cpp

Location: src/tuning/runtime.cpp : 32–33
Description: `#include <utility>` and `#include <iostream>` are present but nothing from `<utility>` (e.g., `std::move`, `std::pair`) or `<iostream>` (`std::cout`, `std::cin`) appears to be used in the file.
Severity rationale: Unused includes increase compile time and obscure actual dependencies.
Suggested fix: Remove `#include <utility>` and `#include <iostream>` from runtime.cpp.

## Review of ethernet.cpp, ethernet.h, wifi.cpp, wifi.h, tuningusb.h — batch 04

### Medium item #1 : `#ifndef` include guard instead of `#pragma once` in ethernet.h

Location: src/tuning/ethernet.h : 25–26
Description: The header uses a traditional `#ifndef _INCLUDE_GUARD_ETHERNET_TUNE_H` / `#define` guard. The project style guide (style.md §4.1) mandates `#pragma once`.
Severity rationale: Style guide violation; `#pragma once` is simpler and less error-prone.
Suggested fix: Replace lines 25–27 and the closing `#endif` with `#pragma once`.

### Medium item #2 : `#ifndef` include guard instead of `#pragma once` in wifi.h

Location: src/tuning/wifi.h : 25–26
Description: Same issue as Medium item #1 — `#ifndef _INCLUDE_GUARD_WIFI_TUNE_H` guard should be `#pragma once`.
Severity rationale: Same as Medium item #1.
Suggested fix: Replace with `#pragma once`.

### Medium item #3 : `#ifndef` include guard instead of `#pragma once` in tuningusb.h

Location: src/tuning/tuningusb.h : 25–26
Description: Same issue — `#ifndef _INCLUDE_GUARD_USB_TUNE_H` guard should be `#pragma once`.
Severity rationale: Same as Medium items #1 and #2.
Suggested fix: Replace with `#pragma once`.

### Medium item #4 : Duplicate and incorrectly-quoted `#include "unistd.h"` in wifi.cpp

Location: src/tuning/wifi.cpp : 28 and 34
Description: `unistd.h` is included twice: first as `#include "unistd.h"` (using quotes, which searches project-local directories first — incorrect for a system header), and again as the correct `#include <unistd.h>` (with angle brackets) at line 34. The first inclusion uses the wrong quoting style and is entirely redundant.
Severity rationale: Wrong quoting for a system header (could accidentally pick up a local file shadowing unistd.h); duplicate include is dead code.
Suggested fix: Remove the `#include "unistd.h"` at line 28 entirely; keep `#include <unistd.h>` at line 34.

### Medium item #5 : Unchecked `ioctl` return values in `toggle()`

Location: src/tuning/ethernet.cpp : 123 and 126
Description: Both `ioctl(sock, SIOCETHTOOL, &ifr)` calls in `toggle()` — the ETHTOOL_GWOL read (line 123) and the ETHTOOL_SWOL write (line 126) — have their return values discarded. If ETHTOOL_GWOL fails, the subsequent ETHTOOL_SWOL write will use zeroed (unread) WoL data. If ETHTOOL_SWOL fails silently, the user receives no indication that the toggle had no effect.
Severity rationale: Silent failure of a user-visible action (toggling WoL), and write based on potentially uninitialised data.
Suggested fix: Check both return values; on failure for ETHTOOL_GWOL abort the toggle; log or surface ETHTOOL_SWOL failure.

### Medium item #6 : readdir loop does not explicitly skip `.` and `..` entries

Location: src/tuning/wifi.cpp : 86–95
Description: The `readdir` loop in `add_wifi_tunables()` does not explicitly skip the "." and ".." directory entries. While the `strstr(entry->d_name, "wlan")` filter happens to exclude them (since "." and ".." do not contain "wlan"), the project rules (rules.md) require that all `opendir`/`readdir` code explicitly skip `.` and `..`. If the filter condition is ever relaxed or changed, the rule violation could become a real bug.
Severity rationale: Explicit rule violation from rules.md; also fragile — depends on an incidental property of the filter rather than a deliberate guard.
Suggested fix:
```cpp
if (entry->d_name[0] == '.')
    continue;
```

## Review of tuningusb.cpp, tuning.cpp, tuning.h, bluetooth.cpp, bluetooth.h — batch 05

### Medium item #1 : Dangling pointer comparison after `closedir()`

Location: src/tuning/tuningusb.cpp : 112–125
Description: The loop assigns `entry = readdir(dir)` on each iteration. When the loop exits via `break` (an interface without autosuspend support is found), `entry` holds a pointer into the DIR's internal buffer. `closedir(dir)` is then called on line 120, freeing that buffer. The comparison `if (entry)` on line 123 reads a dangling pointer. While comparing a dangling pointer to NULL is not a dereference and works in practice on Linux (the pointer value itself is still on the stack), it is undefined behaviour per the C++ standard and relies on the freed memory not having been immediately reused.
Severity rationale: Undefined behaviour; correct only by coincidence on current Linux/glibc. A future allocator or sanitizer will flag this.
Suggested fix: Use a boolean flag:
```cpp
bool has_non_autosuspend = false;
while ((entry = readdir(dir))) {
    if (!isdigit(entry->d_name[0]))
        continue;
    filename = std::format("...", d_name, entry->d_name);
    if (access(filename.c_str(), R_OK) == 0 && read_sysfs(filename) == 0) {
        has_non_autosuspend = true;
        break;
    }
}
closedir(dir);
if (has_non_autosuspend)
    return;
```

### Medium item #2 : `last_check_time` cache not updated on several error paths

Location: src/tuning/bluetooth.cpp : 141–162
Description: The 60-second cache for the expensive `popen` check is guarded by `if (time(NULL) - last_check_time > 60)`. Inside that block, `last_check_time = time(NULL)` is only reached if popen succeeds AND the first `fgets` succeeds AND the second `fgets` succeeds AND `strlen(line) == 0`. Every other path hits `goto out` without updating `last_check_time`. Consequently, if the first `fgets` returns NULL (empty file from hcitool), `popen` is called on every single invocation of `good_bad()` rather than being throttled to once per minute. This defeats the stated "this check is expensive" comment entirely.
Severity rationale: Performance regression — an intentionally throttled system call is called at full rate on a common code path.
Suggested fix: Update `last_check_time` at the top of the block, before the popen call, so that the 60-second window is always honoured regardless of outcome.

### Medium item #3 : Fixed-size `char` buffer used for parsing popen output

Location: src/tuning/bluetooth.cpp : 146–159
Description: `char line[2048]` is used with `fgets(line, 2047, file)`. This is a C-idiom in C++ code. The size 2047 (instead of `sizeof(line)`) also means the buffer is read one byte short of its capacity without explanation. Beyond the style concern, a hypothetically long hcitool output line could be silently truncated, causing the `strlen(line) > 0` check to incorrectly conclude a connection exists.
Severity rationale: Poor C++ practice; the buffer-size mismatch is an off-by-one that could produce incorrect results on long output lines.
Suggested fix: Use `std::string line; std::getline(...)` after wrapping the FILE* in a `std::unique_ptr` or using `__gnu_cxx::stdio_filebuf`, or at minimum change `fgets(line, 2047, file)` to `fgets(line, sizeof(line), file)`.

### Medium item #4 : Unsigned underflow when `all_tunables` is empty

Location: src/tuning/tuning.cpp : 85
Description: `w->cursor_max = all_tunables.size() - 1;` — `size()` returns `size_t` (unsigned). If `all_tunables` is empty (all tunable-detection functions found nothing), `0 - 1` wraps to `SIZE_MAX` (typically 2^64−1 on 64-bit). `cursor_max` is typed `int` in `tab_window`, so the assignment truncates, but the result is still a large positive number. The cursor can then be moved far beyond the end of the vector, causing undefined behaviour when `all_tunables[cursor_pos]` is later dereferenced.
Severity rationale: Potential out-of-bounds vector access if the system has no tunables (e.g., in a restricted container environment).
Suggested fix:
```cpp
w->cursor_max = all_tunables.empty() ? 0 : static_cast<int>(all_tunables.size()) - 1;
```

### Medium item #5 : Outdated `hci_usb` kernel module check

Location: src/tuning/bluetooth.cpp : 128–130
Description: The condition `access("/sys/module/hci_usb", F_OK)` checks for the `hci_usb` kernel module, which was replaced by `btusb` in Linux kernel 2.6.24 (circa 2008). On all modern kernels this path never exists, so `access()` always returns −1 (non-zero). The full guard `(devinfo.flags & 1) == 0 && access("/sys/module/hci_usb", F_OK)` therefore always evaluates to true when the interface is down, and `goto out` is taken, setting result to TUNE_GOOD. While this happens to be the correct result (BT down = power-efficient), the dead check makes the code misleading and could mask future logic errors.
Severity rationale: Dead code that obscures the actual logic; the module it checks has not existed for 15+ years.
Suggested fix: Remove the `access("/sys/module/hci_usb", F_OK)` part of the condition. If the intent is "interface is down → TUNE_GOOD", express it directly:
```cpp
if ((devinfo.flags & 1) == 0)  /* interface is down */
    goto out;
```

### Medium item #6 : Use of `system()` for Bluetooth toggle

Location: src/tuning/bluetooth.cpp : 180–185
Description: `bt_tunable::toggle()` calls `system("/usr/sbin/hciconfig hci0 up &> /dev/null &")` and `system("/usr/sbin/hciconfig hci0 down &> /dev/null")` to toggle the interface. `system()` forks a shell, making the implementation dependent on `/bin/sh`, subject to environment variable manipulation, and unable to safely capture errors. The return value check only tests the shell's own exit code, not hciconfig's. Furthermore, hciconfig is deprecated (same issue as hcitool).
Severity rationale: Use of deprecated tool; `system()` is considered poor practice for programmatic invocation of system commands.
Suggested fix: Use the `HCIDEVDOWN`/`HCIDEVUP` ioctl directly on the already-opened socket, matching the pattern used in `good_bad()` and `add_bt_tunable()`.

## Review of src/tuning/tuningsysfs.cpp, src/tuning/tuningsysfs.h, src/tuning/nl80211.h, src/tuning/iw.h, src/tuning/iw.c — batch 06

### Medium item #1 : Non-thread-safe global variable enable_power_save

Location: src/tuning/iw.c : 124
Description: `static int enable_power_save;` is a file-scoped global that is written by `set_wifi_power_saving()` (before calling `__handle_cmd`) and by the netlink callback `print_power_save_handler`, and read by `set_power_save()` and `get_wifi_power_saving()`. Both public API functions are non-reentrant: if two threads (or two interfaces) invoke `get_wifi_power_saving` or `set_wifi_power_saving` concurrently, the results are unpredictable. Additionally, `get_wifi_power_saving` returns `1` on both an nl80211 init error *and* on a genuine "power saving enabled" state, making callers unable to distinguish the two outcomes.
Severity rationale: Non-reentrant API using shared global state; incorrect result when called concurrently or when initialisation fails.
Suggested fix: Pass the power-save state as an output parameter (or return value), eliminating the global. For the ambiguous-return issue, return a tri-state (`-1` = error, `0` = disabled, `1` = enabled).

### Medium item #2 : User-visible error strings not wrapped in _() for i18n

Location: src/tuning/iw.c : 86, 91, 97, 215, 221, 262
Description: Six `fprintf(stderr, ...)` error messages are not wrapped in the gettext `_()` macro, making them untranslatable. Per `rules.md` and `style.md`, all user-visible strings must use `_()`. Affected strings include "Failed to allocate netlink socket.\n", "Failed to connect to generic netlink.\n", "Failed to allocate generic netlink cache.\n", "failed to allocate netlink message\n", "failed to allocate netlink callbacks\n", and "building message failed\n".
Severity rationale: i18n rule violation; error messages presented to users are not translatable.
Suggested fix: Wrap each string: `fprintf(stderr, _("Failed to allocate netlink socket.\n"));` etc.

### Medium item #3 : `string` used without std:: prefix in tuningsysfs.cpp constructor

Location: src/tuning/tuningsysfs.cpp : 39
Description: The constructor signature uses the bare name `string` three times: `sysfs_tunable::sysfs_tunable(const string &str, const string &_sysfs_path, const string &target_content)`. This compiles only because `tuningsysfs.h` (included earlier) contains the banned `using namespace std;`. Per `style.md` and `general-c++.md`, `std::string` must always be used with its fully qualified name.
Severity rationale: Relies on the `using namespace std` that is itself a rule violation; will break if the namespace pollution is removed.
Suggested fix: Replace all three occurrences of `string` with `std::string` in the constructor signature.

### Medium item #4 : Non-portable kernel header <asm/errno.h> included in userspace code

Location: src/tuning/iw.c : 49
Description: `#include <asm/errno.h>` pulls in an architecture-specific kernel header. Userspace code should use `<errno.h>` (already included at line 32) to access `errno` and the `E*` constants. The kernel `<asm/errno.h>` is not guaranteed to be available or consistent across all build environments and toolchain configurations.
Severity rationale: Non-portable include; `<errno.h>` already provides everything needed.
Suggested fix: Remove line 49 (`#include <asm/errno.h>`); `<errno.h>` on line 32 already provides all needed error constants.

### Medium item #5 : Non-idiomatic empty-check: length() > 0 instead of !empty()

Location: src/tuning/tuningsysfs.cpp : 67
Description: `if (bad_value.length() > 0)` tests whether a `std::string` is non-empty by comparing its length to zero. The idiomatic C++ way is `if (!bad_value.empty())`. The `empty()` call is O(1)-guaranteed by the standard (whereas `length()` is also O(1) in practice for `std::string` but `empty()` expresses intent more clearly) and is the canonical form preferred by `general-c++.md` which says "Strongly prefer STL constructs".
Severity rationale: Non-idiomatic use of std::string API; minor correctness gap.
Suggested fix: `if (!bad_value.empty())`

## Review of tuningi2c.h, tuningi2c.cpp, powerconsumer.h, powerconsumer.cpp, processdevice.h — batch 07

### Medium item #1 : `using namespace std;` in tuningi2c.h

Location: src/tuning/tuningi2c.h : 28
Description: `using namespace std;` appears at file scope in a header. This injects the entire `std` namespace into every translation unit that includes this header, causing silent name collisions and making it impossible for includers to opt out.
Severity rationale: Per style.md §4.2 this is explicitly forbidden in headers; it pollutes the namespace of all downstream TUs.
Suggested fix: Remove the `using namespace std;` line. Replace any bare `string`, `vector`, etc. in the header with their fully-qualified `std::string`, `std::vector` equivalents.

### Medium item #2 : `using namespace std;` in powerconsumer.h

Location: src/process/powerconsumer.h : 33
Description: Same problem as Medium item #1 above — `using namespace std;` at file scope in a header, polluting all includers.
Severity rationale: Same as item #1.
Suggested fix: Remove the `using namespace std;` line and qualify all standard-library names with `std::`.

### Medium item #3 : Unqualified `vector` in `all_power` declaration

Location: src/process/powerconsumer.h : 72
Description: `extern vector <class power_consumer *> all_power;` uses the bare unqualified `vector` name, which only works because of the `using namespace std;` on line 33.  When that `using` is removed (as it should be), this declaration will fail to compile.
Severity rationale: Directly coupled to the `using namespace std` violation; would become a compile error on cleanup.
Suggested fix: `extern std::vector<class power_consumer *> all_power;`

### Medium item #4 : Unqualified `vector` in `all_proc_devices` declaration

Location: src/process/processdevice.h : 50
Description: `extern vector<class device_consumer *> all_proc_devices;` similarly relies on the inherited `using namespace std;` from `powerconsumer.h`.
Severity rationale: Same as item #3.
Suggested fix: `extern std::vector<class device_consumer *> all_proc_devices;`

### Medium item #5 : Non-translatable `"abstract"` strings in `name()` and `type()`

Location: src/process/powerconsumer.h : 60, 61
Description: The default implementations of `name()` and `type()` return bare string literals `"abstract"` without wrapping them in `_()`. If these base-class defaults are ever displayed to the user (e.g. as a fallback when a subclass doesn't override), the strings will be untranslatable.
Severity rationale: Per rules.md, all user-visible strings must use the `_()` gettext pattern.
Suggested fix: Return `_("abstract")` from both methods, or return an empty string if they are truly meant to be abstract placeholders that should never reach the UI.

### Medium item #6 : Non-translatable `"Device"` string in `type()`

Location: src/process/processdevice.h : 42
Description: `virtual std::string type(void) { return "Device"; }` returns a bare untranslated string literal.
Severity rationale: Per rules.md, all user-visible strings must use `_()`.
Suggested fix: `return _("Device");`

### Medium item #7 : Non-translatable unit strings in `usage_units()`

Location: src/process/powerconsumer.cpp : 95, 97, 99
Description: `" µs/s"`, `" us/s"`, and `" ms/s"` are returned as plain string literals. These unit strings are displayed in the PowerTOP UI and are not wrapped in `_()`, making them untranslatable.
Severity rationale: Per rules.md, all user-visible strings must use `_()`.
Suggested fix: Wrap each string in `_()`, e.g. `return _(" ms/s");`.

### Medium item #8 : Unqualified `string` in constructor definition

Location: src/tuning/tuningi2c.cpp : 35
Description: The constructor definition `i2c_tunable::i2c_tunable(const string &path, const string &name, ...)` uses the unqualified `string` type, relying on the `using namespace std;` injected by included headers. This makes the code fragile and non-portable.
Severity rationale: Violates style.md §4.2 (always use `std::` prefix); directly depends on the namespace pollution from the header.
Suggested fix: Change to `const std::string &path, const std::string &name`.

## Review of processdevice.cpp, process.h, process.cpp, work.cpp, work.h — batch 08

### Medium item #1 : Debug printf left in production code, unlocalized, wrong output stream

Location: src/process/process.cpp : 66-67
Description: `printf("%llu time    %llu since \n", ...)` is a diagnostic trace that was never removed. It writes to stdout (not stderr), uses a non-localized format string, and fires on every measurement interval where an out-of-order timestamp is detected. The double-space in the format string also suggests it is unfinished debug output.
Severity rationale: Any user-visible diagnostic string must use `_()` per rules.md; stdout is the wrong stream for error messages; leftover debug output is confusing and noisy in production.
Suggested fix: Remove the printf entirely, or replace with `fprintf(stderr, _("..."), ...)` if the diagnostic is intentional, after first fixing the ordering bug (High item #1).

### Medium item #2 : Explicit `using namespace std;` in work.cpp

Location: src/process/work.cpp : 36
Description: `using namespace std;` is explicitly declared at file scope. This is prohibited by style.md ("Avoid `using namespace std;`") and general-c++.md ("`using namespace std;` is a deprecated construct").
Severity rationale: Direct violation of the project's stated coding rules; also masks which names come from `std::` versus the project.
Suggested fix: Remove the `using namespace std;` line and qualify all standard library names with `std::` (e.g., `std::map`, `std::pair`, `std::for_each`).

### Medium item #3 : `NULL` instead of `nullptr` in C++ code

Location: src/process/process.cpp : 89-90
Description: `last_waker = NULL;` and `waker = NULL;` use the C macro `NULL` instead of the C++ keyword `nullptr`. In C++11 and later (this project targets C++20), `nullptr` is the correct null-pointer constant.
Severity rationale: Minor correctness issue — `NULL` is typically defined as `0` or `(void*)0`, both of which can cause subtle type issues. C++20 code should use `nullptr`.
Suggested fix: Replace both with `nullptr`.

### Medium item #4 : Narrowing conversion from `unsigned long long` to `int` for tgid

Location: src/process/process.cpp : 104
Description: `tgid = std::stoull(line.substr(pos + 1));` assigns an `unsigned long long` result to the `int` field `tgid`. On systems where PIDs approach `INT_MAX` (or if the file contains an unexpected large value) this silently truncates or produces undefined behavior prior to C++20 (implementation-defined in C++20).
Severity rationale: Narrowing implicit conversion; `std::stoi` or a range-checked conversion is more appropriate. Medium because large PIDs are rare in practice but the root cause is incorrect.
Suggested fix: Use `std::stoi` (which already returns `int`) and wrap in the existing try/catch:
```cpp
tgid = std::stoi(line.substr(pos + 1));
```

### Medium item #5 : `#ifndef` header guard instead of `#pragma once` in process.h

Location: src/process/process.h : 25-27
Description: The file uses a traditional `#ifndef _INCLUDE_GUARD_PROCESS_H` / `#define` guard. style.md §4.1 mandates `#pragma once`.
Severity rationale: Direct violation of the project style guide.
Suggested fix: Replace lines 25–27 and line 95 (`#endif`) approach with `#pragma once` at the top of the file.

### Medium item #6 : `#ifndef` header guard instead of `#pragma once` in work.h

Location: src/process/work.h : 25-27
Description: Same as Medium item #5 — `#ifndef _INCLUDE_GUARD_WORK_H` guard instead of `#pragma once`.
Severity rationale: Direct violation of the project style guide.
Suggested fix: Replace with `#pragma once`.

### Medium item #7 : User-facing description strings not wrapped in `_()` for i18n

Location: src/process/process.cpp : 116, 122, 125
Description: The three `std::format` calls that compose the process description strings (`"[PID {}] {}"`, `"[PID {}] [{}]"`) produce user-visible output but are not wrapped with `_()` / `pt_format(_(...), ...)`. rules.md requires all user-visible strings to be translatable.
Severity rationale: Internationalization regression; any non-English-speaking user will see English-only process descriptions.
Suggested fix: Change to `pt_format(_("[PID {}] {}"), pid, comm)` etc., using the `pt_format` helper required by style.md §4.3.

### Medium item #8 : Unqualified `vector` in header process.h

Location: src/process/process.h : 71
Description: `extern vector <class process *> all_processes;` uses `vector` without the `std::` prefix. This compiles only because `powerconsumer.h` pollutes the global namespace via `using namespace std;`, but the declaration itself should be `std::vector`.
Severity rationale: Header files should never rely on namespace pollution from transitively-included headers; this makes the declaration fragile and violates style.md §4.2.
Suggested fix: Change to `extern std::vector<class process *> all_processes;`.

## Review of interrupt.h, interrupt.cpp, timer.cpp, timer.h, do_process.cpp — batch 09

### Medium item #1 : Header guards use #ifndef instead of #pragma once

Location: src/process/interrupt.h : 25  /  src/process/timer.h : 25
Description: Both headers use the old-style `#ifndef _INCLUDE_GUARD_XXX` / `#define` / `#endif` header guards. The project style guide (style.md §4.1) mandates `#pragma once`.
Severity rationale: Style violation; inconsistent with the rest of the codebase.
Suggested fix: Replace with `#pragma once` and remove the `#ifndef`/`#define`/`#endif` triplet.

### Medium item #2 : Unqualified vector in interrupt.h extern declarations

Location: src/process/interrupt.h : 54
Description: The two `extern` declarations use unqualified `vector` instead of `std::vector`. They compile only because `powerconsumer.h` (transitively included) contains `using namespace std;`. The style guide forbids relying on implicit namespace injection.
Severity rationale: Violates `std::` qualification rule; fragile — any reordering of includes could break compilation.
Suggested fix: Change to `extern std::vector<class interrupt *> all_interrupts;` and `extern const std::vector<std::string> softirqs;`.

### Medium item #3 : string::npos used without std:: prefix

Location: src/process/do_process.cpp : 382
Description: `string::npos` is written without the `std::` prefix. It resolves today only via the transitive `using namespace std;` from `powerconsumer.h`. This is a style violation per both style.md §4.2 and general-c++.md.
Severity rationale: Style violation; implicit namespace injection should not be relied upon.
Suggested fix: Change to `std::string::npos`.

### Medium item #4 : timer_list class in timer.h appears to be dead/unused code

Location: src/process/timer.h : 53
Description: `class timer_list` with members `timer_address` and `timer_func` is declared but never referenced in the codebase. Shipping dead class definitions increases maintenance burden and may confuse readers.
Severity rationale: Dead code; no functional impact but adds noise.
Suggested fix: Remove `class timer_list` unless it is intentionally reserved for future use, in which case add a comment.

### Medium item #5 : Type mismatch — find_create_timer(uint64_t) vs map<unsigned long, ...>

Location: src/process/timer.cpp : 71
Description: `all_timers` is declared `map<unsigned long, class timer *>` but `find_create_timer` accepts a `uint64_t` argument and the `timer` constructor takes `unsigned long`. On a 32-bit Linux kernel (where `unsigned long` is 32 bits), passing a 64-bit kernel function address silently truncates it, producing wrong lookups and spurious duplicate timers.
Severity rationale: Incorrect behaviour on 32-bit systems; type mismatch is a latent bug.
Suggested fix: Change `all_timers` to `std::map<uint64_t, class timer *>` and the constructor argument to `uint64_t`.

### Medium item #6 : stderr diagnostic messages not wrapped in _()

Location: src/process/do_process.cpp : 408, 445, 457
Description: Three `fprintf(stderr, ...)` calls emit literal English strings without the `_()` gettext macro. The project rules (rules.md) require all user-visible strings to be translatable. While these are diagnostic/debug messages, they can appear on user terminals and should be translatable.
Severity rationale: i18n policy violation.
Suggested fix: Wrap each string in `_()`.

### Medium item #7 : usage_units_summary returns "%" literal not wrapped in _()

Location: src/process/interrupt.cpp : 89  /  src/process/timer.cpp : 107
Description: Both `usage_units_summary()` implementations return the literal `"%"` without wrapping in `_()`. This string is user-visible (it appears in the overview display and reports). Even if `%` itself needs no translation, the method's contract is to return a units string, and as a pattern this sets a bad precedent for strings that do need translation.
Severity rationale: i18n policy violation (rules.md).
Suggested fix: Return `_("%")` or, if the percent sign is definitively not localised, document that explicitly.

### Medium item #8 : Summary wprintw uses fragmented translated strings — untranslatable as a sentence

Location: src/process/do_process.cpp : 846
Description: The summary display line is built with multiple independent `_()` fragments embedded in a single C format string:
```cpp
wprintw(win, "%s: %3.1f %s,  %3.1f %s, %3.1f %s %3.1f%% %s\n\n",
    _("Summary"), total_wakeups(), _("wakeups/second"), ...);
```
The structural word ordering is fixed by the C format string and cannot be changed by translators. Different languages require different word orders. This pattern makes a correct translation impossible.
Severity rationale: Fundamental i18n correctness issue; directly affects all non-English users.
Suggested fix: Combine into a single complete translatable sentence using `pt_format` with named arguments, or use a single `_()` string that contains all the labels and format specifiers.

## Review of src/perf/perf.h, src/perf/perf_event.h, src/perf/perf_bundle.cpp, src/perf/perf_bundle.h, src/perf/perf.cpp — batch 10

### Medium item #1 : `using namespace std;` in header perf.h

Location: src/perf/perf.h : 37
Description: `using namespace std;` is used in a header file. This pollutes the global namespace for every translation unit that includes this header, directly or transitively. The style guide and general-c++ rules both explicitly prohibit this. All standard-library names should be qualified with `std::`.
Severity rationale: A `using namespace std;` in a header is a project-wide namespace pollution violation. Any identifier in `std::` can silently shadow or conflict with user identifiers in any file that includes this header.
Suggested fix: Remove `using namespace std;` and use `std::string`, `std::vector`, etc. explicitly.

### Medium item #2 : `using namespace std;` in header perf_bundle.h

Location: src/perf/perf_bundle.h : 32
Description: Same issue as Medium item #1 above. `using namespace std;` is used in a header file, polluting the namespace for all includers.
Severity rationale: Same as above.
Suggested fix: Remove `using namespace std;` and qualify `vector` and `map` usages with `std::`.

### Medium item #3 : Non-translatable kernel configuration strings printed to stderr

Location: src/perf/perf.cpp : 105
Description: The string `"CONFIG_PERF_EVENTS=y\nCONFIG_PERF_COUNTERS=y\nCONFIG_TRACEPOINTS=y\nCONFIG_TRACING=y\n"` is a user-visible message printed to stderr but is not wrapped in `_()`. All user-visible strings must be translatable per the project rules.
Severity rationale: User-facing strings not wrapped in `_()` will never be translated.
Suggested fix: Wrap in `_()` or split into separate `fprintf` calls each wrapped in `_()`.

### Medium item #4 : Non-translatable mmap failure message

Location: src/perf/perf.cpp : 120
Description: `fprintf(stderr, "failed to mmap with %d (%s)\n", ...)` — user-visible string not wrapped in `_()`.
Severity rationale: Same as Medium item #3.
Suggested fix: `fprintf(stderr, _("failed to mmap perf buffer: %d (%s)\n"), errno, strerror(errno));`

### Medium item #5 : Non-translatable perf enable failure message

Location: src/perf/perf.cpp : 127
Description: `fprintf(stderr, "failed to enable perf \n");` — user-visible string not wrapped in `_()`.
Severity rationale: Same as Medium item #3.
Suggested fix: `fprintf(stderr, _("failed to enable perf\n"));`

### Medium item #6 : Error output via `cout` instead of `stderr`, non-translatable

Location: src/perf/perf.cpp : 221
Description: `cout << "stop failing\n";` sends an error message to stdout rather than stderr, and the string is not wrapped in `_()` for translation. The project style guide requires `fprintf(stderr, ...)` for error reporting.
Severity rationale: Error messages on stdout can be lost in automated pipelines; the string is also not translatable.
Suggested fix: `fprintf(stderr, _("perf stop failed\n"));`

### Medium item #7 : Non-translatable `printf` error in `handle_trace_point`

Location: src/perf/perf_bundle.cpp : 282
Description: `printf("UH OH... abstract handle_trace_point called\n")` is a user-visible diagnostic message that is not wrapped in `_()`, and uses `printf` instead of `fprintf(stderr, ...)`.
Severity rationale: User-visible string not wrapped in `_()` will never be translated; sending to stdout also violates the error-reporting style.
Suggested fix: `fprintf(stderr, _("abstract handle_trace_point called\n"));`

### Medium item #8 : `perf_event::stop()` calls `ioctl` on potentially invalid file descriptor

Location: src/perf/perf.cpp : 218
Description: `stop()` unconditionally calls `ioctl(perf_fd, PERF_EVENT_IOC_DISABLE, 0)` without first checking whether `perf_fd` is valid (>= 0). If an event was never started (or mmap failed leaving the fd valid but unusable), this can produce unexpected ioctl errors or act on an unintended fd.
Severity rationale: Calling ioctl on fd -1 is safe (returns EBADF) but calling it on a leaked/stale fd could affect unrelated resources.
Suggested fix:
```cpp
void perf_event::stop(void)
{
    if (perf_fd < 0)
        return;
    int ret = ioctl(perf_fd, PERF_EVENT_IOC_DISABLE, 0);
    if (ret)
        fprintf(stderr, _("perf stop failed\n"));
}
```

### Medium item #9 : `trace_type` is `unsigned int` but is assigned `-1` as a sentinel value

Location: src/perf/perf.h : 54, src/perf/perf.cpp : 159, src/perf/perf_bundle.cpp : 112
Description: `trace_type` is declared as `unsigned int` but `set_event_name` assigns `trace_type = -1` (which wraps to `UINT_MAX`), and `perf_bundle::add_event` then checks `(int)ev->trace_type >= 0` to detect this. This is a confusing misuse of an unsigned type as a signed sentinel, relying on implementation-defined signed/unsigned cast behavior.
Severity rationale: The pattern is error-prone and reliant on cast behavior. Using `int` or an explicit `bool valid` flag would be clearer and correct.
Suggested fix: Change `trace_type` to `int` (signed), or add a separate `bool valid` flag, and remove the cast in the add_event check.

## Review of persistent.cpp, parameters.cpp, parameters.h, learn.cpp, report-formatter.h — batch 11

### Medium item #1 : False null-check after `new` in `clone_results` and `clone_parameters`

Location: src/parameters/parameters.cpp : 297 ; src/parameters/parameters.cpp : 318
Description: Both `clone_results` and `clone_parameters` allocate with `new` and then check `if (!b2) return NULL;`. In standard C++, `new` either succeeds or throws `std::bad_alloc`; it never returns a null pointer. The null check therefore never triggers, giving the reader a false sense of safety. If `bad_alloc` is thrown the callers do not handle it, and the function's declared contract (returning nullptr on failure) is not fulfilled. Callers that do check the return value for null will not be protected from an allocation failure.
Severity rationale: The guard is misleading and never executes, making it a latent correctness issue.
Suggested fix: Either remove the null checks and let `std::bad_alloc` propagate, or use `new (std::nothrow)` and keep the null-check if null-return semantics are actually required by callers.

### Medium item #2 : `get_param_directory` returns an empty string when no writable directory exists

Location: src/parameters/parameters.cpp : 459
Description: `get_param_directory` checks two candidate directories and returns `tempfilename`. If neither `/var/cache/powertop` nor `/data/local/powertop` is writable, `tempfilename` is never set and the function returns an empty string. Every caller (`save_parameters`, `load_parameters`, `save_all_results`, `load_results`) then attempts to open a file whose name is `""`, which will fail silently with a confusing empty-pathname error message. There is no fallback (e.g. the current directory or a user home-directory path).
Severity rationale: Silent data loss / confusing errors in a common deployment scenario (e.g. container, read-only /var).
Suggested fix: Return an error indicator or fall back to a default path, and have callers check for an empty return value explicitly.

### Medium item #3 : User-facing string in `global_power_valid` not wrapped in `_()`

Location: src/parameters/parameters.cpp : 450
Description: The `printf` call `printf("To show power estimates do %ld measurement(s) connected to battery only\n", …)` is not wrapped in the `_()` gettext macro. This string is displayed directly to the user but will not be translated. All user-visible strings must use `_()` per rules.md and style.md.
Severity rationale: Breaks internationalisation; message is never translated.
Suggested fix: Change to `printf(_("To show power estimates do %ld measurement(s) connected to battery only\n"), …);`

### Medium item #4 : Error/informational messages written to `cout` (stdout) instead of `stderr`

Location: src/parameters/persistent.cpp : 47, 89, 183, 184
Description: Four diagnostic messages use `cout <<` which writes to stdout. Per style.md ("Use `fprintf(stderr, ...)` for error reporting"), error and warning messages should go to stderr. Writing errors to stdout mixes them with program output and breaks piping / scripting.
Severity rationale: Incorrect output stream for error messages; violates project style.
Suggested fix: Replace `cout << _("…") << …` with `fprintf(stderr, "%s %s\n", _("…"), pathname.c_str());` (or equivalent).

### Medium item #5 : `#include "string.h"` uses quotes and a C header in parameters.h

Location: src/parameters/parameters.h : 33
Description: The inclusion `#include "string.h"` uses double-quotes instead of angle-brackets for a system header, which causes the compiler to search the project include path first. Additionally, `string.h` is the C header; the C++ equivalent is `<cstring>`. In a C++20 project the C header should not be used directly.
Severity rationale: Wrong include style for a system header and wrong header choice for C++.
Suggested fix: Replace with `#include <cstring>` (angle brackets, C++ header) — or remove entirely if not needed.

### Medium item #6 : Final `result_bundle` allocated in `load_results` is always leaked

Location: src/parameters/persistent.cpp : 121
Description: After the last `:` separator line is processed, the code allocates a new `result_bundle` (`bundle = new struct result_bundle;`) in preparation for the next record. If no further records follow (end of file), `bundle_saved` remains `1` from the previous record, so the `if (bundle_saved == 0) delete bundle;` guard at line 139 does NOT delete this final empty allocation. One empty `result_bundle` is leaked on every successful `load_results` call that processes at least one record.
Severity rationale: Memory leak on every load of a non-empty results file.
Suggested fix: After the loop, unconditionally `delete bundle;` (the empty partially-filled bundle should never be kept):
```cpp
delete bundle;   // always discard the last-allocated but unsaved bundle
if (bundle_saved == 0)
    return;      // or keep existing logging
```

## Review of report-formatter-html.h, report.cpp, report.h, report-formatter-csv.h, report-maker.cpp — batch 12

### Medium item #1 : `using namespace std` in implementation file

Location: report.cpp : 43
Description: `using namespace std;` appears at file scope in the implementation file. While less harmful than in a header, the coding guidelines label this a deprecated construct and the style guide prohibits it. All standard-library names should be qualified with `std::`.
Severity rationale: Explicit style violation; introduces risk of silent name collisions.
Suggested fix: Remove the `using namespace std;` line and add `std::` prefixes throughout the file.

### Medium item #2 : Old-style `#ifndef` include guards instead of `#pragma once`

Location: report.h : 26, report-formatter-html.h : 27, report-formatter-csv.h : 26
Description: All three headers use the legacy `#ifndef / #define / #endif` triple instead of `#pragma once` as required by the style guide (§4.1).
Severity rationale: Style-guide violation; `#pragma once` is simpler, less error-prone, and universally supported.
Suggested fix: Replace the guard triple in each header with a single `#pragma once`.

### Medium item #3 : Fixed-size `char buf[128]` for `strftime` output

Location: report.cpp : 104
Description: `get_time_string` uses a 128-byte stack buffer for the `strftime` result. With certain locales and format strings (especially `%c`) the formatted date/time can exceed 127 characters. If it does, `strftime` returns 0 and the function returns an empty string silently rather than producing a truncated or complete result. Per the C++ guidelines, fixed-size buffer usage "must be provably correct to not overflow".
Severity rationale: Not provably safe; an unusually verbose locale `%c` format could produce an empty field in the report without any diagnostic.
Suggested fix: Use a `std::string` with a `strftime`-in-a-loop pattern or a sufficiently large initial estimate:
```cpp
static std::string get_time_string(const std::string &fmt, time_t t)
{
    struct tm *tm_info = localtime(&t);
    if (!tm_info) return "";
    std::string buf(256, '\0');
    size_t n = strftime(buf.data(), buf.size(), fmt.c_str(), tm_info);
    buf.resize(n);
    return buf;
}
```

### Medium item #4 : `private:` access specifier on the same line as a method declaration

Location: report-formatter-html.h : 63, report-formatter-csv.h : 65
Description: In both headers the `private:` label is appended to the end of the last `public` method declaration on the same source line, e.g. `void add_table(...);private:`. This is a formatting error: access specifiers must appear on their own line, consistent with the K&R/Linux-kernel brace style used throughout the project.
Severity rationale: Style violation; degrades readability and makes the visibility boundary easy to miss in diffs.
Suggested fix: Place `private:` on a new line after the semicolon.

### Medium item #5 : Unqualified `string` (and `vector`) in report-maker.cpp

Location: report-maker.cpp : 114, 140
Description: `report-maker.cpp` does not declare `using namespace std` itself; the identifier `string` resolves only because `report-formatter-csv.h` (included earlier) leaks `using namespace std` into the translation unit. The style guide mandates `std::string` everywhere. The same applies to `vector` in signatures that take `const vector<std::string> &`.
Severity rationale: Style violation that silently depends on an indirect `using` from a header; will break if that header is ever cleaned up.
Suggested fix: Replace `const string &` and `const vector<...> &` with `const std::string &` and `const std::vector<...> &` throughout report-maker.cpp.

## Review of report-maker.h, report-data-html.cpp, report-data-html.h, report-formatter-base.cpp, report-formatter-base.h — batch 13

### Medium item #1 : Uninitialized `title_mod` field in `init_tune_table_attr`

Location: src/report/report-data-html.cpp : 96–104
Description: `init_tune_table_attr` sets all fields of `table_attributes` except `title_mod`, which is left at whatever value is in the caller's struct. Every other `init_*_table_attr` function explicitly sets `title_mod = 0`. If the struct is used after partial re-initialisation (i.e. was previously used for a TC/TLC table), `title_mod` will hold a stale non-zero value and may produce incorrect column-span or title-positioning output.
Severity rationale: Uninitialized field that can silently corrupt table rendering.
Suggested fix: Add `table_css->title_mod = 0;` before the `rows`/`cols` assignments.

### Medium item #2 : Uninitialized `title_mod` field in `init_wakeup_table_attr`

Location: src/report/report-data-html.cpp : 106–114
Description: Same issue as Medium item #1 — `title_mod` is not initialized. All other table-attribute initializer functions set this field.
Severity rationale: Same as Medium item #1.
Suggested fix: Add `table_css->title_mod = 0;` to `init_wakeup_table_attr`.

### Medium item #3 : Missing copyright/license header

Location: src/report/report-data-html.h : 1  and  src/report/report-data-html.cpp : 1
Description: Neither file contains the standard project copyright and GPL-2 license header required by style.md §3.1. All other files in the `src/report/` directory carry the header.
Severity rationale: License compliance; all project files are required to carry the copyright notice.
Suggested fix: Add the standard Samsung/PowerTOP copyright header to both files.

### Medium item #4 : `get_type()` is not `const`

Location: src/report/report-maker.h : 112
Description: `get_type()` is a pure accessor — it returns the private `type` field without modifying the object. It is not declared `const`, preventing its use on `const report_maker` references and violating the general C++ rule "use `const` when possible".
Severity rationale: Const-correctness violation; the accessor is callable in contexts where a const reference would be appropriate.
Suggested fix: `report_type get_type() const;`

### Medium item #5 : Non-`const` pointer parameters for read-only struct access in `report_maker`

Location: src/report/report-maker.h : 125, 127, 130
Description: `add_div(struct tag_attr *div_attr)`, `add_title(struct tag_attr *att_title, ...)`, and `add_table(..., struct table_attributes *tb_attr)` all take non-const pointers even though the functions only read the pointed-to data. This prevents passing pointers to `const`-qualified structs and falsely implies mutation.
Severity rationale: Const-correctness rule violation; callers with `const`-qualified objects cannot call these methods.
Suggested fix: Change all three parameter types to `const struct tag_attr *` / `const struct table_attributes *`.

### Medium item #6 : `#ifndef` header guards use reserved identifiers; should use `#pragma once`

Location: src/report/report-maker.h : 26–27  and  src/report/report-formatter-base.h : 26–27
Description: Both files use `#ifndef _REPORT_MAKER_H_` / `#ifndef _REPORT_FORMATTER_BASE_H_`. Identifiers beginning with an underscore followed by an uppercase letter are reserved to the implementation in C++ (per \[lex.name\]), making these macro names technically undefined behaviour. The project style guide (style.md §4.1) mandates `#pragma once` instead.
Severity rationale: Style guide violation; also uses reserved identifiers.
Suggested fix: Replace both `#ifndef`/`#define`/`#endif` guard triples with `#pragma once`.

### Medium item #7 : Virtual overrides in `report_formatter_string_base` lack `override` specifier

Location: src/report/report-formatter-base.h : 34–38
Description: `get_result()`, `clear_result()`, and `add()` are declared `virtual` in `report_formatter` and overridden in `report_formatter_string_base`, but none carry the `override` keyword. Without `override`, a signature mismatch (e.g., from a future base-class refactor) silently creates a new virtual function rather than overriding the intended one.
Severity rationale: C++11 best practice; omitting `override` undermines static verification of virtual dispatch.
Suggested fix: Replace each `virtual Ret method(...)` with `Ret method(...) override` (drop the `virtual` keyword in the derived class).

## Review of report-formatter-csv.cpp, report-formatter-html.cpp, dram_rapl_device.cpp, dram_rapl_device.h, cpu_linux.cpp — batch 14

### Medium item #1 : HTML table/title/summary data inserted without escaping

Location: src/report/report-formatter-html.cpp : 305, 319-320, 351-369
Description: `add_title()`, `add_summary_list()`, and `add_table()` all insert dynamic data (device names, power values, system strings) directly into the HTML output via `add_exact()`, which does **not** call `escape_string()`. The class defines a proper `escape_string()` that handles `<`, `>`, and `&` — but it is only invoked through the base class `add()` method, which none of these formatting methods use. If any device name or power label contains `<`, `>`, or `&` characters, the output HTML will be malformed or contain injected markup.
Severity rationale: Output HTML corruption for any system with device names containing special characters. The class has the right tool (`escape_string`) but consistently fails to use it for dynamic content.
Suggested fix: Replace direct use of `add_exact(std::format(..., data))` with `add_exact(std::format(..., escape_string(data)))` for all dynamic data fields in these methods.

### Medium item #2 : `using namespace std` in a header file

Location: src/cpu/dram_rapl_device.h : 31
Description: `using namespace std;` appears at file scope in the header `dram_rapl_device.h`. This is explicitly prohibited by the style guide ("Avoid `using namespace std;`") and the C++ rules ("'using namespace std;' is a deprecated construct"). Placing it in a header silently forces the `std` namespace on every translation unit that includes this header, potentially causing name collisions in code that has no relation to this class.
Severity rationale: Header-file namespace pollution affects all includers in the entire project, not just this file.
Suggested fix: Remove `using namespace std;` and use `std::string`, `std::vector` etc. explicitly throughout the header.

### Medium item #3 : `parse_cstates_start` / `parse_cstates_end` do not explicitly skip `.` and `..` entries

Location: src/cpu/cpu_linux.cpp : 64, 143
Description: Both `parse_cstates_start` and `parse_cstates_end` use `if (strlen(entry->d_name) < 3) continue;` as an implicit way to skip `.` and `..`. The PowerTOP-specific rules explicitly require: "When using opendir/readdir or equivalent, the code must skip the '.' and '..' entries in the directory and not process them as normal." The current check only works incidentally because both `.` and `..` happen to have lengths less than 3; it is not a deliberate skip, and the intent is not clear to a reader. A future refactor of the minimum-length check could inadvertently re-expose `.`/`..` processing.
Severity rationale: Violates an explicit project rule. The implicit skip is fragile and unclear.
Suggested fix: Add an explicit check before or instead of the length check:
```cpp
if (entry->d_name[0] == '.')
    continue;
```

### Medium item #4 : `add_quotes()` declared but never defined

Location: src/report/report-formatter-csv.h : 66
Description: `add_quotes()` is declared as a private member of `report_formatter_csv` but has no definition anywhere in the codebase. It is also never called. This represents dead/incomplete code: the method cannot be linked if ever called, and the related state variables (`csv_need_quotes`, `text_start`) are orphaned. This suggests a feature was partially sketched and never completed or was deleted without cleaning up the declaration.
Severity rationale: Dead declaration paired with uninitialized state variables indicates incomplete implementation that could cause a linker error if the function is ever invoked.
Suggested fix: Either implement `add_quotes()` properly (initializing `csv_need_quotes` and `text_start`), or remove the declaration, the member variables, and the assignment in `escape_string()`.

## Review of src/cpu/cpu_core.cpp, src/cpu/cpudevice.h, src/cpu/cpudevice.cpp, src/cpu/abstract_cpu.cpp, src/cpu/cpu_rapl_device.cpp — batch 15

### Medium item #1 : Error reporting via `std::cout` instead of `fprintf(stderr, ...)`

Location: src/cpu/abstract_cpu.cpp : 275, 375
Description: Two error conditions — an invalid C-state finalize and an invalid P-state finalize —
are reported by writing to `std::cout` (stdout). The project style guide (style.md §6) mandates
`fprintf(stderr, ...)` for all error reporting. Using stdout means these messages are silently
swallowed when the user redirects output (e.g. `powertop > report.txt`) and they also intermix
with normal output in the ncurses UI.
Severity rationale: Violates the explicit error-reporting rule; errors can be lost in typical
usage scenarios.
Suggested fix: Replace both `cout << "Invalid C state finalize ..."` and
`cout << "Invalid P state finalize ..."` with `fprintf(stderr, "Invalid C state finalize %s\n", linux_name.c_str())` and `fprintf(stderr, "Invalid P state finalize %" PRIu64 "\n", freq)` respectively.

### Medium item #2 : `std::stoull` result silently narrowed to `int line_level`

Location: src/cpu/abstract_cpu.cpp : 220, 247
Description: `state->line_level` is declared as `int` (see cpu.h `struct idle_state`).
Both call sites use `std::stoull()` which returns `unsigned long long`, then assign it directly
to the `int` field. This is a narrowing conversion: if the parsed number is larger than
`INT_MAX` the result is implementation-defined (effectively UB in C++20 for values outside
the destination range). The correct function to use for an `int` target is `std::stoi`.
Severity rationale: Silent narrowing / potential UB; `std::stoi` is the semantically correct
choice and any out-of-range value will throw `std::out_of_range` which is already caught by
the surrounding `catch (...)` block.
Suggested fix: Replace both occurrences of `std::stoull(...)` with `std::stoi(...)`.

### Medium item #3 : User-visible strings `"CPU core"` not wrapped in `_()` for translation

Location: src/cpu/cpu_rapl_device.h : 48, 49
Description: `device_name()` and `human_name()` both return the hard-coded string `"CPU core"`.
Both methods are virtual overrides in the device hierarchy whose return values are displayed
to the user. The project rule (rules.md, style.md §5) requires all user-visible strings to be
wrapped in the `_()` gettext macro to be translatable.
Severity rationale: User-visible strings that are not wrapped in `_()` cannot be localised;
this is an explicit project rule violation.
Suggested fix: Change both return statements to `return _("CPU core");`.
Note: since these are inline definitions in the header, `libintl.h` / the gettext macro must
be available at the include site; consider moving the definitions to `cpu_rapl_device.cpp`.

## Review of src/cpu/cpu_rapl_device.h, src/cpu/cpu_package.cpp, src/cpu/cpu.h, src/cpu/cpu.cpp, src/cpu/intel_cpus.cpp — batch 16

### Medium item #1 : Missing negative-cpunr check in handle_trace_point

Location: src/cpu/cpu.cpp : 948
Description: The bounds check `if (cpunr >= (int)all_cpus.size())` only guards the
upper bound. If `cpunr` is negative (e.g. due to perf data corruption or an
unexpected event), the check passes (negative < positive) and
`all_cpus[cpunr]` accesses memory before the vector's buffer — undefined
behaviour that can corrupt heap metadata or crash.
Severity rationale: While perf normally gives non-negative CPU numbers, the
defensive check is trivial and its absence is a latent safety issue.
Suggested fix: `if (cpunr < 0 || cpunr >= (int)all_cpus.size()) return;`

### Medium item #2 : User-facing strings in cpu_rapl_device.h not wrapped in _()

Location: src/cpu/cpu_rapl_device.h : 48-49
Description: Both `device_name()` (returns `"CPU core"`) and `human_name()`
(returns `"CPU core"`) return string literals without the gettext `_()` macro.
These strings are displayed to the user in the device list and power report.
Per the project rules, all user-visible strings must be wrapped in `_()`.
Severity rationale: Untranslatable user-visible strings; breaks i18n for all locales.
Suggested fix: `return _("CPU core");` in both methods.

### Medium item #3 : `#ifndef` header guards instead of `#pragma once`

Location: src/cpu/cpu_rapl_device.h : 25-26  and  src/cpu/cpu.h : 26-27
Description: Both headers use traditional `#ifndef`/`#define`/`#endif` include
guards. The project style guide mandates `#pragma once` for all header files.
Severity rationale: Style violation explicitly called out in style.md §4.1.
Suggested fix: Replace the `#ifndef ... #define ... #endif` guards with a single `#pragma once`.

### Medium item #4 : `#include <format>` placed mid-file

Location: src/cpu/cpu_package.cpp : 41  and  src/cpu/intel_cpus.cpp : 345
Description: The `#include <format>` directive appears after several other
includes and function definitions. The project style guide requires all includes
to appear at the top of the file, grouped as standard library headers first then
project headers.
Severity rationale: Style violation; also makes the dependency on `<format>` easy to miss.
Suggested fix: Move `#include <format>` to the top-of-file include block in both files.

### Medium item #5 : Error output via `cout` instead of `fprintf(stderr, ...)`

Location: src/cpu/cpu.cpp : 949
Description: `cout << "INVALID cpu nr in handle_trace_point\n"` uses C++ stream
output to stdout for an error condition. The project style guide (§6) specifies
`fprintf(stderr, ...)` for error reporting. Additionally the string is not wrapped
in `_()` for translation.
Severity rationale: Error goes to stdout (mixed with normal output), and is
untranslatable; violates both the error-handling and i18n rules.
Suggested fix: `fprintf(stderr, _("INVALID cpu nr in handle_trace_point\n"));`

### Medium item #6 : `NULL` instead of `nullptr` in default parameter

Location: src/cpu/cpu_rapl_device.h : 46
Description: The constructor default parameter uses `NULL` (`class abstract_cpu *_cpu = NULL`).
C++20 code should use `nullptr` for null pointer constants.
Severity rationale: `NULL` is a C-style macro; `nullptr` is the C++11+ type-safe alternative and the correct form for C++20 code.
Suggested fix: `class abstract_cpu *_cpu = nullptr`

### Medium item #7 : `core_num` variable is always zero — dead-code branch

Location: src/cpu/cpu.cpp : 436, 613
Description: `core_num` is declared and initialised to `0` at line 436 and is
never incremented anywhere in `report_display_cpu_cstates`. The branch
`if (core_num > 0)` on line 613 is therefore permanently dead code; the title
normalisation that was presumably intended never executes.
Severity rationale: Logic bug; the title-row calculation is silently wrong for packages with multiple cores.
Suggested fix: Either increment `core_num` alongside `num_cores` or remove the dead branch and unify the title computation.

### Medium item #8 : TSC divide-by-zero in residency ratio computation

Location: src/cpu/intel_cpus.cpp : 313, 604, 684
Description: In `nhm_core::measurement_end`, `nhm_package::measurement_end`, and
`nhm_cpu::measurement_end` the ratio is computed as:
`ratio = 1.0 * time_delta / (tsc_after - tsc_before);`
If `tsc_after == tsc_before` (possible on some hypervisors or immediately after
system resume where TSC is reset), the denominator is zero. For double division
this yields `Inf` or `NaN` which then silently propagates into all
`duration_delta`/`usage_delta` values, corrupting the displayed data.
Severity rationale: Silent data corruption on hypervisors or post-resume; no
user-visible error is produced.
Suggested fix: Guard with `if (tsc_after != tsc_before) { ratio = ...; }` and
skip the loop or set deltas to zero otherwise.

## Review of src/cpu/intel_gpu.cpp, src/cpu/rapl/rapl_interface.cpp, src/cpu/rapl/rapl_interface.h, src/wakeup/wakeup.h — batch 17

### Medium item #1 : `using namespace std` in a header file

Location: src/wakeup/wakeup.h : 33
Description: The header declares `using namespace std;` at global scope. This injects the entire `std` namespace into every translation unit that includes `wakeup.h`, directly and transitively, causing potential name collisions and bypassing the project rule (style.md §4.2) that forbids `using namespace std`. Any downstream `.cpp` file that includes this header loses the protection of explicit namespace qualification.
Severity rationale: Header-level `using namespace std` is an explicit project style violation that silently affects all includers and can hide naming conflicts throughout the codebase.
Suggested fix: Remove the `using namespace std;` directive from the header and use `std::string`, `std::vector` explicitly throughout the file.

### Medium item #2 : `string` used without `std::` qualifier in function parameter (intel_gpu.cpp)

Location: src/cpu/intel_gpu.cpp : 57
Description: The function definition `std::string i965_core::fill_cstate_line(int line_nr, const string &separator __unused)` uses the unqualified `string` for the parameter type, while the corresponding declaration in `intel_cpus.h` correctly uses `std::string`. This relies on `using namespace std` being injected transitively (likely from `wakeup.h`), which is itself a style violation.
Severity rationale: Direct style violation per style.md §4.2/4.3; introduces implicit dependency on transitive namespace pollution.
Suggested fix: Replace `const string &separator` with `const std::string &separator`.

### Medium item #3 : `string` used without `std::` qualifier in class member declarations (rapl_interface.h)

Location: src/cpu/rapl/rapl_interface.h : 32–34
Description: Member variables `powercap_core_path`, `powercap_uncore_path`, and `powercap_dram_path` are declared as plain `string` without the `std::` prefix. The constructor declaration on line 54 in the same header correctly uses `std::string`. The unqualified `string` will only compile if `using namespace std` is brought in by a previously included header, creating a fragile implicit dependency.
Severity rationale: Style violation per style.md §4.2/4.3; the header has no `#include <string>` of its own, relying entirely on transitive pollution.
Suggested fix: Qualify all three as `std::string` and add `#include <string>` to the header.

### Medium item #4 : Missing `#include <string>` and `#include <cstdint>` in rapl_interface.h

Location: src/cpu/rapl/rapl_interface.h : 24 (after header guard)
Description: `rapl_interface.h` uses `string` (lines 32–34) and `uint64_t` (lines 43–44, 64–82) but includes neither `<string>` nor `<stdint.h>` / `<cstdint>`. The file compiles only because other headers included before it in the translation unit happen to bring in these symbols. Any change to include ordering could silently break compilation.
Severity rationale: Self-contained headers are a fundamental correctness requirement; missing includes create brittle build dependencies.
Suggested fix: Add `#include <string>` and `#include <cstdint>` to `rapl_interface.h`.

### Medium item #5 : `atof()` used without error detection for sysfs energy values

Location: src/cpu/rapl/rapl_interface.cpp : 376, 467, 554
Description: Three call sites convert sysfs energy strings to `double` using `atof(str.c_str())`. `atof` returns 0.0 on invalid input with no error indication, meaning a corrupt or unexpected sysfs value (e.g., an empty re-read race, or a kernel change to the format) would silently be treated as zero energy. This causes the downstream power-watt computation to report 0 W without any diagnostic.
Severity rationale: Silent failure mode for a core measurement path; `stod` or `strtod` would allow error detection.
Suggested fix: Replace `atof(str.c_str())` with `std::stod(str)` wrapped in a try/catch, or use `strtod` with an end-pointer check, and log/return an error on failure.

### Medium item #6 : Redundant MSR reads — `get_energy_status_unit()` called instead of cached member

Location: src/cpu/rapl/rapl_interface.cpp : 300, 390, 481, 569
Description: Each of the four energy-status read functions (`get_pkg_energy_status`, `get_dram_energy_status`, `get_pp0_energy_status`, `get_pp1_energy_status`) calls `get_energy_status_unit()` to scale the raw MSR value. This method re-reads `MSR_RAPL_POWER_UNIT` on every invocation, performing a redundant MSR read. The constructor already reads and caches this value into the `energy_status_units` member variable (line 181). The cached value is never actually used for energy scaling.
Severity rationale: Unnecessary MSR reads on every measurement cycle; using the cached `energy_status_units` member is both more efficient and more consistent.
Suggested fix: Replace `get_energy_status_unit()` at those four sites with the cached `energy_status_units` member variable.

## Review of wakeup_usb.h, wakeup_ethernet.h, waketab.cpp, wakeup_ethernet.cpp, wakeup.cpp — batch 18

### Medium item #1 : `using namespace std` in wakeup.cpp and waketab.cpp

Location: src/wakeup/wakeup.cpp : 32  and  src/wakeup/waketab.cpp : 16
Description: Both .cpp files contain `using namespace std;` at file scope. The project style guide explicitly prohibits this construct, and the code already uses fully-qualified `std::` names for most things. It also creates confusion: `string` on line 59 of `wakeup_ethernet.cpp` is resolved only because wakeup.h (transitively) injects `std::`.
Severity rationale: Medium — direct violation of the style guide; in .cpp files the blast radius is confined to that translation unit, but it is still explicitly prohibited.
Suggested fix: Remove the `using namespace std;` lines and use `std::` everywhere.

### Medium item #2 : TOCTOU race between counting and writing disabled-device rows in report_show_wakeup

Location: src/wakeup/waketab.cpp : 131-159
Description: `report_show_wakeup` calls `wakeup_value()` in a first loop to count disabled entries and dimension `wakeup_data`, then calls `wakeup_value()` again in a second loop to fill the vector. `wakeup_value()` reads from sysfs at runtime; if a device changes state between the two loops (hotplug, runtime PM, etc.) the second loop may write more rows than were allocated, causing an out-of-bounds `std::vector` access (undefined behaviour). Additionally the variable is named `tgb` in the first loop and `gb` in the second, making the relationship non-obvious.
Severity rationale: Medium — the race is real and reachable (sysfs values can change at any time), and the consequence is UB that could corrupt heap or crash.
Suggested fix: Capture all wakeup values once into a local `std::vector<int>` before either loop, then use the cached values for both counting and writing.

### Medium item #3 : spaces used instead of tabs for indentation in wakeup.cpp

Location: src/wakeup/wakeup.cpp : 39-50
Description: The body of both `wakeup` constructors uses leading spaces (4-space indent) for the member assignments (`desc =`, `wakeup_enable =`, etc.) instead of the required tabs. The rest of the codebase uses tabs exclusively.
Severity rationale: Medium — direct violation of the mandatory indentation rule; makes diffs noisy and inconsistent with the rest of the file.
Suggested fix: Replace the leading spaces with a single tab character on each affected line.

## Review of src/measurement/acpi.cpp, src/measurement/opal-sensors.cpp — batch 19

### Medium item #1 : `using namespace std;` in acpi.cpp

Location: src/measurement/acpi.cpp : 36
Description: `acpi.cpp` explicitly includes `using namespace std;` at file scope. The coding style guide explicitly forbids this construct. All standard library identifiers must be referenced with the `std::` prefix.
Severity rationale: Medium — explicit style-guide violation; promotes hidden name collisions and makes code harder to read and maintain.
Suggested fix: Remove `using namespace std;`. Replace bare `string`, `getline`, `istringstream` usages with their `std::` qualified forms.

### Medium item #2 : Integer overflow risk in `opal_sensors_power_meter::power()`

Location: src/measurement/opal-sensors.cpp : 39-45
Description: `read_sysfs()` returns a plain `int` (32-bit signed integer). OPAL sensors report power in microwatts. For a system consuming more than ~2147 W (INT_MAX / 1,000,000), the raw sysfs value would overflow a 32-bit signed integer before the division by 1,000,000.0 is applied. IBM POWER systems targeted by this driver can easily have power supplies exceeding this threshold when considering total system power. The overflow produces undefined behaviour and an incorrect (potentially negative) power reading.
Severity rationale: Medium — affects correctness on high-power POWER systems; produces UB from signed integer overflow.
Suggested fix: Change `value` to `long` or `int64_t` and update `read_sysfs()` call accordingly, or use a string-based read and parse with `strtoll()`.

## Review of sysfs.h, sysfs.cpp, measurement.h, measurement.cpp, extech.h — batch 20

### Medium item #1 : `end_thread` flag not atomic in extech_power_meter

Location: src/measurement/extech.h : 38
Description: `int end_thread` is written by the main thread (`end_measurement` sets it to 1) and read by the sampling thread in the `while (!end_thread)` loop, with no synchronization. Without `std::atomic` or a memory barrier the compiler and CPU are free to cache the value in a register and the sampling thread may never observe the update, looping forever.
Severity rationale: Can cause `pthread_join` in `end_measurement` to hang indefinitely, blocking the entire measurement cycle.
Suggested fix: Change to `std::atomic<bool> end_thread{false};` and update all uses accordingly.

### Medium item #2 : Missing `override` specifier on virtual overrides in sysfs.h

Location: src/measurement/sysfs.h : 46-50
Description: `start_measurement`, `end_measurement`, `power`, and `dev_capacity` all override virtual methods from `power_meter` but none use the `override` keyword. Without `override`, a signature mismatch silently creates a new virtual function instead of overriding the intended one.
Severity rationale: Compiler cannot catch accidental signature mismatches; incorrect overrides go undetected.
Suggested fix: Add `override` to all four declarations, e.g. `virtual void start_measurement(void) override;`.

### Medium item #3 : Missing `override` specifier on virtual overrides in extech.h

Location: src/measurement/extech.h : 41-47
Description: `start_measurement`, `end_measurement`, `power`, and `dev_capacity` override `power_meter` methods but lack the `override` keyword.
Severity rationale: Same as medium item #2 above.
Suggested fix: Add `override` to all four method declarations.

### Medium item #4 : `#pragma once` not used — old-style include guards in three headers

Location: src/measurement/measurement.h : 25-26; src/measurement/sysfs.h : 25-26; src/measurement/extech.h : 25-26
Description: All three headers use `#ifndef / #define / #endif` guards instead of `#pragma once` required by style.md §4.1. The guards also use double-underscore prefixes (`__INCLUDE_GUARD_*`) which are reserved identifiers in C++.
Severity rationale: Style violation and use of reserved names; `#pragma once` is simpler and avoids reserved-name UB.
Suggested fix: Replace each `#ifndef / #define / #endif` triple with `#pragma once` at the top of the file.

### Medium item #5 : `extech_power_meter` allocated without `std::nothrow` and without null check

Location: src/measurement/measurement.cpp : 183
Description: `meter = new class extech_power_meter(devnode);` uses throwing `new`, unlike the three similar allocations on lines 144, 152, and 165 which use `new(std::nothrow)` and check for null before pushing to `power_meters`. If allocation fails here the unhandled exception terminates the process; if `meter` were somehow null it would be pushed into `power_meters` and later dereferenced.
Severity rationale: Inconsistency with surrounding code; missing null check creates potential null-pointer dereference.
Suggested fix: Change to `meter = new(std::nothrow) extech_power_meter(devnode); if (meter) power_meters.push_back(meter);`.

## Review of src/measurement/extech.cpp, src/devices/backlight.h, src/devices/backlight.cpp, src/devices/rfkill.cpp — batch 21

### Medium item #1 : `using namespace std;` in extech.cpp, backlight.cpp, rfkill.cpp

Location: src/measurement/extech.cpp : 64 | src/devices/backlight.cpp : 34 | src/devices/rfkill.cpp : 35
Description: All three `.cpp` files contain `using namespace std;`, which is explicitly prohibited by both style.md §4.2 and general-c++.md. This pollutes the global namespace and can cause silent name collisions. The codebase already uses `std::` prefixes consistently in other files.
Severity rationale: Violation of two separate coding-style rules; potential for subtle naming conflicts in a project that mixes C and C++ headers.
Suggested fix: Remove all three `using namespace std;` directives and prefix all standard-library names with `std::`.

### Medium item #2 : `decode_extech_value` return type is `unsigned int` but returns -1 on error

Location: src/measurement/extech.cpp : 145, 201
Description: The function is declared as returning `unsigned int`, yet the error path does `return -1;`. Due to implicit conversion, -1 becomes `UINT_MAX`. The caller checks `if (ret)` which accidentally works because `UINT_MAX != 0`, but the interface is semantically incorrect, misleading, and fragile. Any caller that compares the return value to -1 directly will get the wrong result.
Severity rationale: Incorrect return type; error-detection relies on accidental unsigned arithmetic, a silent API contract violation.
Suggested fix: Change the return type to `int` (or return a negative constant like `-1` explicitly as `int`).

### Medium item #3 : parse_packet inner decode loop runs only once instead of four times

Location: src/measurement/extech.cpp : 222
Description: `for (i = 0; i < 1; i++)` iterates exactly once (i=0 only), decoding only the first five-byte block. The outer loop confirms that four blocks are expected. If all four blocks carry measurement data, only one-quarter of the available information is decoded; the remaining three blocks are silently discarded.
Severity rationale: Potential logic error that limits the measurement to only the first of four data blocks, possibly discarding the actual watts field if it lives in a later block.
Suggested fix: Change the loop bound to match the outer loop: `for (i = 0; i < 4; i++)`. Verify the op[] buffer is large enough (currently 32 bytes; 4 × 8 = 32 bytes — exactly fits).

### Medium item #4 : `human_name()` returns non-translated string "Display backlight"

Location: src/devices/backlight.h : 52
Description: The `human_name()` method returns the literal string `"Display backlight"` without wrapping it in the `_()` gettext macro, violating the project rule that all user-visible strings must be translatable.
Severity rationale: User-visible device name is never translated; breaks i18n for all backlight entries.
Suggested fix: `virtual std::string human_name(void) { return _("Display backlight"); };`

### Medium item #5 : rfkill humanname fallback is not translated

Location: src/devices/rfkill.cpp : 56
Description: When the `readlink()` calls both fail, `humanname` remains set to `std::format("radio:{}", _name)`, a hard-coded non-translated string. The translated form is only used when readlink succeeds (lines 64, 69). The fallback is therefore never localized.
Severity rationale: User-visible device label is untranslated for any rfkill device whose driver symlink cannot be resolved.
Suggested fix: Use a translated fallback, e.g. `humanname = pt_format(_("Radio device: {}"), _name);`.

### Medium item #6 : Error messages in parse_packet use printf() and are not translated

Location: src/measurement/extech.cpp : 217, 227, 291
Description: Three error messages use `printf()` to write to stdout rather than `fprintf(stderr, ...)` as required by style.md §6. None of the strings are wrapped in `_()`. The message at line 291 in `measure()` also uses `printf` instead of `fprintf(stderr,...)`.
Severity rationale: Violates the project style rule for error output; strings are not translatable.
Suggested fix: Replace with `fprintf(stderr, _("Invalid packet\n"));` etc. (or simply remove diagnostic noise for internal protocol errors and return the error code silently).

### Medium item #7 : Hardcoded `/sys/class/drm/card0` in dpms_screen_on()

Location: src/devices/backlight.cpp : 69, 80, 84
Description: `dpms_screen_on()` opens `/sys/class/drm/card0` and then constructs paths under `/sys/class/drm/card0/…`. On systems with multiple GPUs or where the primary display adapter is not `card0` (e.g., discrete NVIDIA where Intel is `card1`), the function will always report the screen as off, causing backlight utilization to be reported as 0% regardless of actual state.
Severity rationale: Directly causes incorrect backlight power estimates on a common class of multi-GPU laptops — a material impact on measurement accuracy.
Suggested fix: Enumerate all `/sys/class/drm/card*` top-level directories and search for an enabled+On connector under each of them, rather than hardcoding `card0`.

## Review of src/devices/usb.h, src/devices/usb.cpp, src/devices/gpu_rapl_device.h, src/devices/gpu_rapl_device.cpp, src/devices/ahci.h — batch 22

### Medium item #1 : `consumed_power` is uninitialized in gpu_rapl_device constructor

Location: src/devices/gpu_rapl_device.cpp : 31-41
Description: The constructor initializes `device_valid(false)` explicitly but never initializes `consumed_power`. In C++, non-static member variables of POD types are left with indeterminate values unless explicitly initialized. If `power_usage()` is called before `end_measurement()` has ever run (which sets `consumed_power = 0.0`), and the RAPL domain is present, the function returns the indeterminate value — undefined behaviour. The fix is to initialize `consumed_power` to `0.0` in the member initializer list.
Severity rationale: UB via use of uninitialized memory; could produce garbage power readings on the first call.
Suggested fix: Add `consumed_power(0.0)` to the member initializer list: `gpu_rapl_device::gpu_rapl_device(i915gpu *parent) : i915gpu(), consumed_power(0.0), device_valid(false)`.

### Medium item #2 : Non-translatable user-facing strings "GPU core" in gpu_rapl_device.h

Location: src/devices/gpu_rapl_device.h : 47-49
Description: The methods `class_name()`, `device_name()`, and `human_name()` all return the hard-coded string `"GPU core"` without the `_()` gettext macro. `human_name()` in particular is the string shown to the user in device listings. The project rules (`rules.md`, `style.md §5`) require that all user-visible strings be wrapped with `_()` for internationalization. The other devices (usb, ahci) already use `pt_format(_("..."), ...)` for their human-readable names.
Severity rationale: Breaks i18n for all users with non-English locales; explicitly required by project rules.
Suggested fix: Change the return expressions: `return _("GPU core");` for `human_name()`. `class_name()` and `device_name()` are internal identifiers — if they are truly user-visible, apply `_()` as well; otherwise document that they are internal.

### Medium item #3 : Missing `override` specifier on virtual methods in derived classes

Location: src/devices/usb.h : 50-62, src/devices/gpu_rapl_device.h : 47-53, src/devices/ahci.h : 53-66
Description: All three derived-class headers redeclare base-class virtual methods using the `virtual` keyword but omit `override`. In C++11 and later, `override` causes a compile-time error if the function signature does not match a virtual function in the base class, catching accidental signature mismatches. Without `override`, silent shadowing bugs (e.g., a parameter type mismatch) compile without warning. The project uses C++20 where `override` is idiomatic.
Severity rationale: Absence of `override` is a C++ best-practice violation that can hide real signature bugs silently.
Suggested fix: Replace `virtual` with `override` (or add `override` after the parameter list) on every overriding method declaration. Remove the leading `virtual` keyword as it is redundant when `override` is present.

### Medium item #4 : Fixed-size C-style buffer in `register_power_with_devlist`

Location: src/devices/usb.cpp : 115-118
Description: `char devfs_name[1024]` is used with `snprintf` to produce a path of the form `"usb/NNN/NNN"`. The project rules (`general-c++.md`) require preferring `std::string` over C-style strings and char arrays except when interfacing with C-only libraries. `register_devpower` accepts a `const char *` (C interface), but the intermediate buffer should be avoided. The buffer is far larger than necessary and is a maintenance hazard.
Severity rationale: Style/rule violation; the buffer is not at risk of overflow in practice, but fixed-size buffers are explicitly discouraged and the project has the std::format infrastructure to do this without a buffer.
Suggested fix: Replace with `std::string devfs_name = std::format("usb/{:03}/{:03}", busnum, devnum);` and pass `devfs_name.c_str()` to `register_devpower`.

### Medium item #5 : `start_measurement` / `end_measurement` call RAPL API without checking `device_valid`

Location: src/devices/gpu_rapl_device.cpp : 43-61
Description: Both `start_measurement()` and `end_measurement()` unconditionally call `rapl.get_pp1_energy_status()` regardless of whether `device_valid` is true. If the RAPL pp1 domain is not present, the call is a no-op in benign implementations, but the code relies on that assumption. Additionally, `last_energy` is only initialized in the constructor's `if (rapl.pp1_domain_present())` branch; if the domain is absent, `last_energy` is uninitialized and `end_measurement` would compute a meaningless `consumed_power` before writing it back.
Severity rationale: Can interact with the uninitialized `last_energy` issue (M1) to produce UB or incorrect readings.
Suggested fix: Guard both methods: `if (!device_valid) return;` at the top of `start_measurement()` and `end_measurement()`.

### Medium item #6 : Unqualified `string` in usb.cpp constructor signature

Location: src/devices/usb.cpp : 41
Description: The constructor definition uses `const string &_name, const string &path, const string &devid` — bare `string` without the `std::` prefix. This compiles only because some transitively-included header introduces `using namespace std` (likely `gpu_rapl_device.h` via the include chain, or some system header). The project style (`style.md §4.2`, `general-c++.md`) requires explicit `std::` qualification throughout. The header declaration at `usb.h:48` correctly uses `std::string`.
Severity rationale: Code relies on namespace pollution from another header to compile; fragile and style non-compliant.
Suggested fix: Change line 41 to `usbdevice::usbdevice(const std::string &_name, const std::string &path, const std::string &devid)`.


## Review of src/devices/ahci.cpp, src/devices/thinkpad-fan.h, src/devices/thinkpad-fan.cpp, src/devices/device.h, src/devices/device.cpp — batch 23

### Medium item #1 : `using namespace std;` in ahci.cpp and device.cpp

Location: src/devices/ahci.cpp : 34 ; src/devices/device.cpp : 35
Description: Both source files contain `using namespace std;` at file scope. While less harmful than the identical construct in device.h (it does not affect other translation units), it is still explicitly prohibited by style.md §4.2 ("Avoid `using namespace std;`") and general-c++.md. The rest of the codebase uses the `std::` prefix consistently.
Severity rationale: Style and correctness violation in two files; does not affect other TUs but sets a bad example and can hide name conflicts within the file.
Suggested fix: Remove the `using namespace std;` lines and add `std::` prefixes where needed (the existing code already uses `std::` for most calls, so the change is minimal).

### Medium item #2 : User-visible string "Laptop fan" not wrapped with `_()`

Location: src/devices/thinkpad-fan.h : 48
Description: The `human_name()` virtual method returns the hard-coded string `"Laptop fan"`. This string is displayed directly to the user in the device power table (it becomes the "Device name" column entry). Per rules.md: "All user visible strings must be translatable and use the _() gettext pattern". The string is currently not translatable.
Severity rationale: Direct i18n violation for a string that is always shown to users on ThinkPad systems.
Suggested fix: `virtual std::string human_name(void) { return _("Laptop fan"); };`

### Medium item #3 : Override virtual methods lack `override` specifier

Location: src/devices/thinkpad-fan.h : 40–52
Description: All virtual methods in `thinkpad_fan` that override `device` base-class methods (`start_measurement`, `end_measurement`, `utilization`, `class_name`, `device_name`, `human_name`, `power_usage`, `util_units`, `power_valid`, `grouping_prio`) are declared with `virtual` but without the C++11 `override` keyword. The `override` specifier lets the compiler catch signature mismatches between override and base.
Severity rationale: Missing `override` is a C++ best practice violation; a future signature change in the base class would silently stop the override from working.
Suggested fix: Replace `virtual void start_measurement(void);` with `void start_measurement(void) override;` (and similarly for all other overrides). Drop the redundant `virtual` when `override` is present.

### Medium item #4 : Old-style `#ifndef` include guards instead of `#pragma once`

Location: src/devices/thinkpad-fan.h : 25–26 ; src/devices/device.h : 25–26
Description: Both headers use the traditional `#ifndef _INCLUDE_GUARD_…` / `#define` / `#endif` pattern. style.md §4.1 states: "Use `#pragma once`".
Severity rationale: Style guide violation in two headers.
Suggested fix: Replace the `#ifndef` / `#define` / `#endif` trio with a single `#pragma once` directive.

### Medium item #5 : Duplicate `#include <unistd.h>` in thinkpad-fan.cpp and device.cpp

Location: src/devices/thinkpad-fan.cpp : 32 and 44 ; src/devices/device.cpp : 32 and 56
Description: `<unistd.h>` is included twice in each of these files. In thinkpad-fan.cpp it appears at lines 32 and 44; in device.cpp at lines 32 and 56. Duplicate includes are harmless due to include guards but indicate copy-paste errors and reduce readability.
Severity rationale: Code quality / maintenance issue.
Suggested fix: Remove the second occurrence of `#include <unistd.h>` in each file.

### Medium item #6 : Unused includes `<iostream>` and `<fstream>` in thinkpad-fan.cpp

Location: src/devices/thinkpad-fan.cpp : 25–26
Description: `<iostream>` and `<fstream>` are included but neither `std::cout`, `std::cin`, `std::ifstream`, nor any other entity from these headers is used anywhere in thinkpad-fan.cpp. This increases compilation time and obscures the file's true dependencies.
Severity rationale: Code clarity violation; may mislead future maintainers into thinking stream I/O is being used.
Suggested fix: Remove lines 25–26 (`#include <iostream>` and `#include <fstream>`).

### Medium item #7 : Override virtual methods in device.h lack `override` specifier

Location: src/devices/device.h : 45–72
Description: The base class `device` declares all its interface methods as `virtual`. While being the base class it is not required to use `override` itself, derived classes (e.g. `thinkpad_fan`, `ahci`) do not use `override` either. This is a pervasive issue across the hierarchy — noting it at the base class level for context.
Severity rationale: Same rationale as Medium item #3; a pattern that should be adopted hierarchy-wide.
Suggested fix: Add `override` to all override declarations in all derived device classes.

### Medium item #8 : Unused variable `cols` and `rows` default-initialized to 0 then immediately overwritten

Location: src/devices/ahci.cpp : 262–264
Description: `int cols=0; int rows=0;` are declared with initializers, then two lines later unconditionally assigned `cols=5; rows=links.size()+1;`. The zero-initialization is dead code that can confuse the reader.
Severity rationale: Minor code clarity issue; initializers 0 are never observable.
Suggested fix: Declare and initialise at point of use:
```cpp
int cols = 5;
int rows = static_cast<int>(links.size()) + 1;
```
## Review of src/devices/devfreq.cpp, src/devices/devfreq.h, src/devices/alsa.h, src/devices/alsa.cpp, src/devices/runtime_pm.cpp — batch 24

### Medium item #1 : Old-style `#ifndef` header guards instead of `#pragma once`

Location: src/devices/devfreq.h : 25, src/devices/alsa.h : 25, src/devices/runtime_pm.h : 25
Description: All three headers use the `#ifndef _INCLUDE_GUARD_xxx` / `#define` / `#endif` idiom. style.md §4.1 mandates `#pragma once` for header guards in this project.
Severity rationale: Style rule violation across all three headers reviewed.
Suggested fix: Replace the `#ifndef`/`#define`/`#endif` triple in each header with a single `#pragma once` at the top.

### Medium item #2 : User-visible string `"Idle"` not wrapped in `_()`

Location: src/devices/devfreq.cpp : 97
Description: `state->human_name = "Idle";` assigns a hard-coded English string that is displayed directly in the ncurses UI and reports. All user-visible strings must use the `_()` gettext macro per style.md §5 and rules.md.
Severity rationale: String cannot be localised; violates mandatory i18n rule.
Suggested fix: `state->human_name = _("Idle");`

### Medium item #3 : Untranslated user-visible error messages sent to stderr

Location: src/devices/devfreq.cpp : 239, 249
Description: Two calls `fprintf(stderr, "Devfreq not enabled\n")` output English-only text. While these go to stderr, they are user-visible diagnostic messages that should be translatable per the project's i18n rules. Similarly, the ncurses string `" Devfreq is not enabled"` at line 278 is already correctly wrapped but the stderr counterpart is not.
Severity rationale: Inconsistency; stderr messages are user-visible and must be localisable.
Suggested fix: `fprintf(stderr, _("Devfreq not enabled\n"));`

### Medium item #4 : `uint64_t` arithmetic underflow in `alsa::end_measurement()` numerator

Location: src/devices/alsa.cpp : 107
Description: `(end_active - start_active)` is computed as `uint64_t - uint64_t`. If `end_active < start_active` (e.g., after a counter reset, driver reload, or suspend/resume), the result silently wraps to a very large `uint64_t`. When this large value is then divided by the (correctly computed double) denominator, `p` becomes a huge number (>> 100%) which gets stored and displayed as the utilisation. The same pattern also appears in `alsa::utilization()` at line 116.
Severity rationale: Silent data corruption on counter wrap produces wildly incorrect power accounting.
Suggested fix: Cast to signed 64-bit before subtraction, or clamp: `double active_delta = (end_active >= start_active) ? (double)(end_active - start_active) : 0.0;`

### Medium item #5 : `dstates` is a public member of `devfreq`

Location: src/devices/devfreq.h : 48
Description: `vector<class frequency *> dstates` is declared `public`, allowing any external code to modify the internal state vector directly. `display_devfreq_devices()` in devfreq.cpp iterates over it externally (`df->dstates`). Exposing the raw vector violates encapsulation and makes it harder to change the implementation.
Severity rationale: Design defect that exposes memory-managed internals; coupled to at least one direct external use.
Suggested fix: Make `dstates` private and add a `size_t freq_state_count()` and `const frequency* freq_state(size_t idx)` accessor pair.

### Medium item #6 : `static int index = 0` lazy-init fails when `get_param_index` returns 0

Location: src/devices/alsa.cpp : 146-150
Description: `power_usage()` uses a static local `index` initialised to 0 as a sentinel for "not yet looked up". If `get_param_index("alsa-codec-power")` legitimately returns 0 (i.e., it is the first registered parameter), the guard `if (!index)` is always true and the index is re-fetched on every call. At worst this is an O(n) lookup on every power calculation; at best it is silently correct by accident.
Severity rationale: Logic error with a subtle trigger condition; incorrect parameter lookup on every power calculation if index happens to be 0.
Suggested fix: Use `static int index = -1;` as the sentinel, or initialise the index in the constructor.

### Medium item #7 : `do_bus()` adds all bus devices to device list without runtime PM check

Location: src/devices/runtime_pm.cpp : 130-174
Description: `do_bus()` creates a `runtime_pmdevice` for every entry in `/sys/bus/{bus}/devices/` unconditionally, including devices that have no `power/runtime_suspended_time` or `power/runtime_active_time` sysfs files. The helper `device_has_runtime_pm()` exists precisely to detect this but is never called inside `do_bus()`. Consequences: (1) potentially dozens of ghost devices appear in the UI with permanently 0% utilisation; (2) a power parameter entry is registered for each via `register_parameter(humanname)`, bloating the parameter registry.
Severity rationale: Functional correctness issue — non-PM devices are silently included and pollute the device list and parameter registry.
Suggested fix: After constructing `dev`, check `device_has_runtime_pm(path)` and `delete dev; continue;` if false, before pushing to `all_devices`.

### Medium item #8 : `active_time` accumulator compared against `sample_time` without overflow guard

Location: src/devices/devfreq.cpp : 82
Description: In `process_time_stamps()`, `dstates[i]->time_after` (type `uint64_t`) is assigned `sample_time - active_time` where `sample_time` is `double` (microseconds) and `active_time` is a `uint64_t` accumulation of nanosecond-domain values (line 79 multiplies by 1000). If measurement jitter or counter imprecision causes `active_time > sample_time`, the double subtraction yields a negative value which is then silently truncated to a very large `uint64_t` on assignment — corrupting the idle-time accounting for the device.
Severity rationale: Unsigned underflow on double-to-uint64_t conversion produces invalid idle time displayed to the user.
Suggested fix:
```cpp
double idle = sample_time - (double)active_time;
dstates[i]->time_after = (idle > 0.0) ? (uint64_t)idle : 0;
```
