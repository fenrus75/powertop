# PowerTOP Low Severity Review Findings

## Review of calibrate.cpp, calibrate.h, lib.cpp, devlist.cpp, main.cpp — batch 01

### Low item #1 : `calibrate.h` uses `#ifndef` guard instead of `#pragma once`

Location: src/calibrate/calibrate.h : 25–27

Description:
The header uses a traditional include guard:
```cpp
#ifndef __INCLUDE_GUARD_CALIBRATE_H
#define __INCLUDE_GUARD_CALIBRATE_H
```
The project style guide (style.md §4.1) mandates `#pragma once` for all headers.

Severity rationale: Style violation; `#pragma once` is the project standard and is universally supported by the compilers in use.

Suggested fix: Replace with `#pragma once` and remove the `#endif` at the bottom.

---

### Low item #2 : `calibrate.h` uses `std::string` without including `<string>`

Location: src/calibrate/calibrate.h : 28

Description:
The declaration `void one_measurement(int, int, const std::string &)` references `std::string` but the header does not include `<string>`. Any translation unit that includes `calibrate.h` before pulling in `<string>` itself will fail to compile. Headers must be self-contained.

Severity rationale: Latent compilation error; currently masked because all callers happen to include `<string>` earlier.

Suggested fix: Add `#include <string>` to `calibrate.h`.

---

### Low item #3 : Duplicate `#include` directives in lib.cpp

Location: src/lib.cpp : 35, 51, 37, 53, 55, 56, 36, 62

Description:
The following headers are included more than once:
- `<stdio.h>` — lines 35 and 51
- `<stdlib.h>` — lines 37 and 53
- `<sys/types.h>` — lines 55 and 56 (consecutive duplicates)
- `<math.h>` — lines 36 and 62

While harmless due to include guards, duplicate includes are noise and may indicate copy-paste accumulation.

Severity rationale: Code quality / maintenance issue; violates the principle of clean, intentional includes.

Suggested fix: Remove the duplicate include directives; keep one of each.

---

### Low item #4 : `setrlimit` return value not checked

Location: src/main.cpp : 369

Description:
```cpp
rlmt.rlim_cur = rlmt.rlim_max = get_nr_open();
setrlimit(RLIMIT_NOFILE, &rlmt);
```
The return value of `setrlimit` is silently discarded. If the call fails, PowerTOP will run with a lower file-descriptor limit than required, potentially causing spurious "too many open files" errors later when iterating `/proc/PID/fd/`.

Severity rationale: Unchecked system-call failure can lead to confusing downstream errors.

Suggested fix:
```cpp
if (setrlimit(RLIMIT_NOFILE, &rlmt) != 0)
    fprintf(stderr, _("Failed to raise open file limit: %s\n"), strerror(errno));
```

---

### Low item #5 : `mkdir` return values not checked in `powertop_init`

Location: src/main.cpp : 402–404

Description:
```cpp
if (access("/var/cache/", W_OK) == 0)
    mkdir("/var/cache/powertop", 0600);
else
    mkdir("/data/local/powertop", 0600);
```
Neither `mkdir` call checks its return value. If directory creation fails for any reason other than the directory already existing (`EEXIST`), the subsequent `load_results` / `save_parameters` calls will silently fail to find or write their data files.

Severity rationale: Silent failure to create the cache directory; saved calibration data may be quietly lost.

Suggested fix: Check the return value and ignore only `EEXIST`:
```cpp
if (mkdir("/var/cache/powertop", 0600) != 0 && errno != EEXIST)
    fprintf(stderr, _("Failed to create cache directory: %s\n"), strerror(errno));
```

---

### Low item #6 : `blmax` global is overwritten for each backlight device

Location: src/calibrate/calibrate.cpp : 152–154

Description:
```cpp
static int blmax;
...
static void find_backlight_callback(const std::string &d_name)
{
    ...
    backlight_devices.push_back(filename);
    filename = std::format("/sys/class/backlight/{}/max_brightness", d_name);
    blmax = read_sysfs(filename);
}
```
`blmax` is a single global. When multiple backlight devices exist, each call to the callback overwrites it. `backlight_calibration` then uses this single `blmax` value for all devices, but writes it back only to the device that was current at calibration time, not to each device's own max. The brightness levels written (`blmax/4`, `blmax/2`, etc.) will be wrong for all but the last discovered device.

Severity rationale: Incorrect calibration data for multi-backlight systems.

Suggested fix: Store `(device_path, max_brightness)` pairs together, e.g., using a `std::vector<std::pair<std::string,int>>`.

---

### Low item #7 : Hardcoded `"wlan0"` wireless interface name

Location: src/calibrate/calibrate.cpp : 77, 385, 393, 408

Description:
The interface name `"wlan0"` is hardcoded in `restore_all_sysfs`, `calibrate`, and the `set_wifi_power_saving` calls. On modern Linux systems the interface may be named `wlp2s0`, `wlan1`, or something else entirely. If `"wlan0"` does not exist, `get_wifi_power_saving` / `set_wifi_power_saving` will silently do nothing (or fail silently), and the saved PS state will be incorrect.

Severity rationale: Calibration will incorrectly handle Wi-Fi power saving on systems without a `wlan0` interface; no error is reported.

Suggested fix: Auto-detect the active wireless interface (e.g., by scanning `/sys/class/net/*/wireless/`) rather than hardcoding `"wlan0"`.

## Review of src/test_framework.h, src/test_framework.cpp, src/display.cpp, src/display.h, src/lib.h — batch 02

### Low item #1 : `NULL` used instead of `nullptr` in lib.h

Location: src/lib.h : 81
Description: `bool *ok = NULL` uses the C-style null pointer constant `NULL`. In C++11+ (and especially C++20) `nullptr` is the correct null pointer literal, providing type safety. `NULL` is typically `0` or `(void*)0`, which can cause overload ambiguity.
Severity rationale: C++ best-practice violation; no runtime risk in this specific context but wrong idiom.
Suggested fix: `bool *ok = nullptr`

---

### Low item #2 : `create_tab()` `bottom_line` parameter passed by value

Location: src/display.h : 96
Description: `void create_tab(const string &name, const string &translation, class tab_window *w = NULL, string bottom_line = "")` — the `bottom_line` parameter is passed by value (copy), requiring a heap allocation for every non-empty string argument. It should be `const std::string&` consistent with the other string parameters.
Severity rationale: Unnecessary copy; inconsistent with surrounding API.
Suggested fix:
```cpp
void create_tab(const std::string &name, const std::string &translation,
                tab_window *w = nullptr, const std::string &bottom_line = "");
```

---

### Low item #3 : `tab_windows` map holds raw owning pointers with no cleanup

Location: src/display.h : 91, src/display.cpp : 41
Description: `map<string, class tab_window *> tab_windows` stores raw owning pointers to `tab_window` objects. These are never deleted when the map is destroyed or when tabs are removed. The destructor of `tab_window` deletes the ncurses window (`delwin(win)`), so proper ownership is needed. Using `std::unique_ptr<tab_window>` would enforce automatic cleanup.
Severity rationale: Memory leak and potential use-after-free if tabs are ever programmatically replaced.
Suggested fix: Change to `std::map<std::string, std::unique_ptr<tab_window>> tab_windows;` and adjust call sites.

---

### Low item #4 : Magic sentinel string `"__POWERTOP_FILE_NOT_FOUND__"` should be a named constant

Location: src/test_framework.cpp : 52, 62, 125, 159
Description: The special sentinel value `"__POWERTOP_FILE_NOT_FOUND__"` is used in four places but defined nowhere as a constant. If it is ever changed in one location and not another, the record/replay mechanism silently breaks.
Severity rationale: Maintainability risk; magic string repeated in multiple locations.
Suggested fix:
```cpp
static constexpr const char* FILE_NOT_FOUND_SENTINEL = "__POWERTOP_FILE_NOT_FOUND__";
```

---

### Low item #5 : `new(class tab_window)` — misleading placement-new syntax

Location: src/display.cpp : 49
Description: `w = new(class tab_window);` uses the parenthesised `(type-id)` form of `new`, which is valid C++ but visually identical to placement-new syntax `new(ptr) T`. Any reader encountering `new(...)` expects a placement expression, not a regular heap allocation. The elaborated type specifier `class tab_window` inside the parentheses adds further confusion.
Severity rationale: Readability/maintenance issue; no runtime risk.
Suggested fix: `w = new tab_window();`

---

### Low item #6 : `replay_write()` logs mismatch to `cerr` but does not throw — silent test failure

Location: src/test_framework.cpp : 72–85
Description: When a write mismatch is detected during replay, the function prints to `cerr` and returns normally. The calling code continues executing as if the write was correct. Contrast with `replay_read()` and `replay_msr()`, which throw `test_exception` on unexpected conditions. This inconsistency means write-mismatch test failures are easy to miss, especially in automated pipelines that do not scan `stderr`.
Severity rationale: Silent test failure undermines the reliability of the test framework.
Suggested fix: Replace the `cerr` output with `throw test_exception(...)`.

---

### Low item #7 : "PowerTOP %s" version header string not translatable

Location: src/display.cpp : 119
Description: `mvwprintw(tab_bar, 0,0, "PowerTOP %s", PACKAGE_VERSION)` prints a static English string. While the product name is typically not translated, the format string itself is not wrapped in `_()`, inconsistent with the project i18n policy and preventing translators from reordering tokens if needed.
Severity rationale: Minor i18n policy violation.
Suggested fix: `mvwprintw(tab_bar, 0, 0, _("PowerTOP %s"), PACKAGE_VERSION);`

## Review of src/devlist.h, src/tuning/tunable.h, src/tuning/tunable.cpp, src/tuning/runtime.h, src/tuning/runtime.cpp — batch 03

### Low item #1 : Missing copyright/license header in devlist.h

Location: src/devlist.h : 1
Description: `devlist.h` has no copyright and GPL license header. Every file in the project is required to carry the standard Intel copyright block per style.md §3.1.
Severity rationale: License compliance issue; all other headers in the project carry the required header.
Suggested fix: Add the standard copyright/license block at the top of the file (same template as in tunable.h, runtime.h, etc.).

### Low item #2 : `#ifndef`/`#define` header guards instead of `#pragma once` in devlist.h

Location: src/devlist.h : 1–2, 29
Description: `devlist.h` uses the old-style `#ifndef __INCLUDE_GUARD_DEVLIST_H__` / `#define` / `#endif` guard. The project style (style.md §4.1) mandates `#pragma once`.
Severity rationale: Style violation; inconsistent with the stated project standard.
Suggested fix: Replace the three guard lines with `#pragma once`.

### Low item #3 : `#ifndef`/`#define` header guards instead of `#pragma once` in tunable.h

Location: src/tuning/tunable.h : 25–26, 92
Description: Same violation as Low #2. `tunable.h` uses `#ifndef _INCLUDE_GUARD_TUNABLE_H`.
Severity rationale: Style violation.
Suggested fix: Replace with `#pragma once`.

### Low item #4 : `#ifndef`/`#define` header guards instead of `#pragma once` in runtime.h

Location: src/tuning/runtime.h : 25–26, 48
Description: Same violation as Low #2 and Low #3. `runtime.h` uses `#ifndef _INCLUDE_GUARD_RUNTIME_TUNE_H`.
Severity rationale: Style violation.
Suggested fix: Replace with `#pragma once`.

### Low item #5 : Constructors do not use member initializer lists in tunable.cpp

Location: src/tuning/tunable.cpp : 35–52
Description: Both constructors assign to member variables in the constructor body (`desc = str;`, `score = _score;`, etc.) rather than using a member initializer list. In C++, member initializer lists are preferred as they initialize members directly rather than default-constructing them first and then assigning.
Severity rationale: C++ best-practice violation per general-c++.md. For `std::string` members this causes a default construction followed by a copy-assignment instead of a single copy construction.
Suggested fix: Use member initializer lists, e.g.:
```cpp
tunable::tunable(const std::string &str, double _score,
                 const std::string &good, const std::string &bad,
                 const std::string &neutral)
    : desc(str), score(_score),
      good_string(good), bad_string(bad), neutral_string(neutral)
{}
```

### Low item #6 : `while (1)` instead of `while (true)` in runtime.cpp

Location: src/tuning/runtime.cpp : 133
Description: `while (1)` is a C idiom. C++ code should use `while (true)` for clarity and to avoid implicit int-to-bool conversion.
Severity rationale: C++ style violation per general-c++.md.
Suggested fix: Change `while (1)` to `while (true)`.

### Low item #7 : Redundant `extern` keyword on function declarations in devlist.h

Location: src/devlist.h : 21–27
Description: All function declarations in `devlist.h` are prefixed with `extern`. In C++, free functions declared at namespace scope are externally linked by default; the `extern` keyword is redundant and adds noise.
Severity rationale: Low severity style inconsistency; no functional impact.
Suggested fix: Remove the `extern` keyword from all function declarations in the header.

## Review of ethernet.cpp, ethernet.h, wifi.cpp, wifi.h, tuningusb.h — batch 04

### Low item #1 : C-style casts `(caddr_t)` in ethernet.cpp

Location: src/tuning/ethernet.cpp : 86 and 122
Description: `ifr.ifr_data = (caddr_t)&wol;` uses a C-style cast. In C++ code, `reinterpret_cast` should be used when casting between unrelated pointer types to make the intent explicit and to be caught by static analysis tools.
Severity rationale: C++ best-practice violation; C-style casts bypass type-safety checks.
Suggested fix: `ifr.ifr_data = reinterpret_cast<caddr_t>(&wol);`

### Low item #2 : `interf` member is `public` in `ethernet_tunable`

Location: src/tuning/ethernet.h : 36
Description: The `interf` data member is declared `public`, exposing the internal interface name of the tunable to all callers. It is only used internally by the class's own methods. Encapsulation calls for it to be `private` (or at most `protected`).
Severity rationale: Unnecessary exposure of internals; violates encapsulation; matches the pattern used in wifi_tunable where `iface` is correctly `private`.
Suggested fix: Move `std::string interf;` to the `private:` section.

### Low item #3 : Redundant `class` keyword in local variable declarations

Location: src/tuning/ethernet.cpp : 134 and src/tuning/wifi.cpp : 80, 91
Description: `class ethernet_tunable *eth` and `class wifi_tunable *wifi` use the elaborated type specifier `class`. In C++ this is unnecessary and unusual; `ethernet_tunable *eth` and `wifi_tunable *wifi` are idiomatic.
Severity rationale: Low-impact style violation; not idiomatic C++.
Suggested fix: Remove the redundant `class` keyword from all three variable declarations.

### Low item #4 : Unused includes `<utility>` and `<iostream>` in ethernet.cpp

Location: src/tuning/ethernet.cpp : 33 and 34
Description: Neither `<utility>` (std::pair, std::move, etc.) nor `<iostream>` (std::cout, std::cin, etc.) symbols are used anywhere in ethernet.cpp. These unused includes add unnecessary compilation overhead and obscure the file's real dependencies.
Severity rationale: Dead code; same pattern flagged in previous batches.
Suggested fix: Remove `#include <utility>` and `#include <iostream>` from ethernet.cpp.

### Low item #5 : Unused includes `<utility>` and `<iostream>` in wifi.cpp

Location: src/tuning/wifi.cpp : 31 and 32
Description: Same as Low item #4 — `<utility>` and `<iostream>` are included but nothing from them is used in wifi.cpp.
Severity rationale: Same as Low item #4.
Suggested fix: Remove `#include <utility>` and `#include <iostream>` from wifi.cpp.

## Review of tuningusb.cpp, tuning.cpp, tuning.h, bluetooth.cpp, bluetooth.h — batch 05

### Low item #1 : System header included with quotes instead of angle brackets

Location: src/tuning/tuningusb.cpp : 28
Description: `#include "unistd.h"` uses double-quote syntax for a system header. Double-quote includes first search the local directory, which can accidentally shadow a system header with a local file of the same name. System headers must use `<unistd.h>`.
Severity rationale: Incorrect include style; harmless in practice but violates the include-order style rule.
Suggested fix: Change to `#include <unistd.h>`.

### Low item #2 : Duplicate `#include` directives

Location: src/tuning/bluetooth.cpp : 28 and 35 (`<unistd.h>`), 36 and 38 (`<sys/types.h>`)
Description: Both `<unistd.h>` and `<sys/types.h>` are included twice in bluetooth.cpp. While include guards make this harmless at the preprocessor level, it is dead code and adds confusion.
Severity rationale: Unnecessary noise; style issue.
Suggested fix: Remove the duplicate `#include <unistd.h>` at line 35 and the duplicate `#include <sys/types.h>` at line 38.

### Low item #3 : `strcpy` into a fixed-size struct field

Location: src/tuning/bluetooth.cpp : 123 and 209
Description: `strcpy(devinfo.name, "hci0")` copies into `hci_dev_info::name[8]`. While "hci0\0" is only 5 bytes so no overflow occurs here, using `strcpy` into a fixed-size field is fragile — a future change to the device name (e.g., "hci10\0") or the field size could silently overflow. The project style guide prefers `std::string` over C-style char arrays.
Severity rationale: Technically safe with the current string, but unsafe pattern; style violation.
Suggested fix: Use `strncpy(devinfo.name, "hci0", sizeof(devinfo.name) - 1); devinfo.name[sizeof(devinfo.name)-1] = '\0';` or restructure to avoid the C struct entirely using BlueZ D-Bus APIs.

### Low item #4 : Non-translatable user-visible error string

Location: src/tuning/bluetooth.cpp : 181 and 185
Description: `printf("System is not available\n")` outputs a user-visible string without the `_()` gettext wrapper. Per the project's i18n rules (rules.md, style.md §5), all user-facing strings must be wrapped in `_()`. Additionally, error output should go to `stderr` not `stdout`, and `printf` should be replaced with `fprintf(stderr, ...)`.
Severity rationale: Violates mandatory i18n rule; error goes to wrong stream.
Suggested fix:
```cpp
fprintf(stderr, _("Bluetooth toggle failed: system() call unavailable\n"));
```

### Low item #5 : Mixed tabs and spaces indentation in `report_show_tunables`

Location: src/tuning/tuning.cpp : 207–215
Description: The `report_show_tunables` function mixes 8-space indentation with tab indentation in adjacent lines (e.g., lines 209–213 use spaces where the rest of the file uses tabs). The project style guide mandates tabs-only indentation.
Severity rationale: Style violation; makes the file inconsistent and harder to read in editors configured for different tab widths.
Suggested fix: Convert the space-indented lines to use a single tab at the appropriate nesting level.

### Low item #6 : `#ifndef` header guard instead of `#pragma once`

Location: src/tuning/tuning.h : 26–27; src/tuning/bluetooth.h : 25–26
Description: Both headers use traditional `#ifndef`/`#define`/`#endif` include guards. The project style guide (style.md §4.1) mandates `#pragma once`.
Severity rationale: Style guide violation; `#pragma once` is simpler and less prone to guard-name collisions.
Suggested fix: Replace each include-guard triplet with a single `#pragma once` at the top of the file.

### Low item #7 : Unused `#include` directives in bluetooth.cpp

Location: src/tuning/bluetooth.cpp : 32–34
Description: `#include <utility>`, `#include <iostream>`, and `#include <fstream>` are present but nothing from these headers (`std::move`, `std::cout`, `std::ifstream`, etc.) is used in bluetooth.cpp.
Severity rationale: Dead includes add compile-time cost and mislead readers about the file's dependencies.
Suggested fix: Remove the three unused include lines.

### Low item #8 : User-visible notification string not wrapped in `_()`

Location: src/tuning/tuning.cpp : 157
Description: `ui_notify_user(std::format(">> {}\n", toggle_script))` passes a format string literal that is not wrapped in `_()`. The `">> {}\n"` part is displayed to the user in the Tunables tab when a tunable is toggled. Per the i18n rules, user-visible strings must use the gettext `_()` macro.
Severity rationale: i18n rule violation; the string is not translatable.
Suggested fix: `ui_notify_user(pt_format(_(">> {}\n"), toggle_script));`

## Review of src/tuning/tuningsysfs.cpp, src/tuning/tuningsysfs.h, src/tuning/nl80211.h, src/tuning/iw.h, src/tuning/iw.c — batch 06

### Low item #1 : Old-style #ifndef header guard in tuningsysfs.h instead of #pragma once

Location: src/tuning/tuningsysfs.h : 25–26
Description: The header uses `#ifndef _INCLUDE_GUARD_SYSFS_TUNE_H` / `#define _INCLUDE_GUARD_SYSFS_TUNE_H`. Per `style.md` section 4.1, the project convention is `#pragma once`.
Severity rationale: Style violation; not a functional bug but inconsistent with project convention.
Suggested fix: Replace lines 25–26 and 51 with `#pragma once` at the top of the file.

### Low item #2 : Old-style #ifndef header guard in iw.h instead of #pragma once

Location: src/tuning/iw.h : 1–2
Description: The header uses `#ifndef __IW_H` / `#define __IW_H`. Per `style.md` section 4.1, the project convention is `#pragma once`. Also, the guard name `__IW_H` starts with a double underscore, which is a reserved identifier in C/C++.
Severity rationale: Style violation and use of reserved identifier name for the macro.
Suggested fix: Replace with `#pragma once` and remove the closing `#endif /* __IW_H */`.

### Low item #3 : Include ordering in tuningsysfs.cpp mixes project and system headers

Location: src/tuning/tuningsysfs.cpp : 26–37
Description: Per `style.md` section 4.4, standard library headers should appear before project-specific headers. In `tuningsysfs.cpp` the order is: `"tuning.h"` (project), `"tunable.h"` (project), `<unistd.h>` (system), `"tuningsysfs.h"` (project), `<string.h>` (system), `<dirent.h>` (system), `<utility>` (stdlib), `<iostream>` (stdlib), `<fstream>` (stdlib), `<limits.h>` (system), `<format>` (stdlib), then `"../lib.h"` (project). System and project headers are interleaved rather than grouped.
Severity rationale: Style violation; ordering makes include dependencies harder to track.
Suggested fix: Group standard library/system headers first, then project headers.

### Low item #4 : Unused include <limits.h> in tuningsysfs.h

Location: src/tuning/tuningsysfs.h : 29
Description: `#include <limits.h>` is present but nothing from `<limits.h>` (e.g. `PATH_MAX`, `CHAR_BIT`) is used anywhere in the header.
Severity rationale: Dead code; increases compilation dependencies unnecessarily.
Suggested fix: Remove the `#include <limits.h>` line.

### Low item #5 : Unused include <vector> in tuningsysfs.h

Location: src/tuning/tuningsysfs.h : 28
Description: `#include <vector>` is present but `std::vector` is not used in the header itself. The vector operations on `all_tunables` are in the `.cpp` file which includes the appropriate headers transitively.
Severity rationale: Unused include; increases compilation overhead.
Suggested fix: Remove the `#include <vector>` line.

### Low item #6 : Unused include <limits.h> in tuningsysfs.cpp

Location: src/tuning/tuningsysfs.cpp : 35
Description: `#include <limits.h>` is included in the `.cpp` file but no symbol from it (`PATH_MAX`, etc.) is used in `tuningsysfs.cpp`.
Severity rationale: Dead code; unnecessary compile-time dependency.
Suggested fix: Remove line 35.

### Low item #7 : ETH_ALEN defined without a #ifndef guard in iw.h

Location: src/tuning/iw.h : 37
Description: `#define ETH_ALEN 6` is an unconditional definition. `ETH_ALEN` is also defined in `<linux/if_ether.h>` and in some libnl headers. If iw.h is included after any of those headers, the compiler will emit a macro-redefinition warning (or error with `-Werror`). Additionally, `ETH_ALEN` is never used anywhere in the reviewed files, making the definition dead code.
Severity rationale: Potential redefinition conflict; unused dead code.
Suggested fix: Either remove the definition entirely (it is unused), or guard it: `#ifndef ETH_ALEN\n#define ETH_ALEN 6\n#endif`.

### Low item #8 : Redundant `class` keyword in pointer declarations in add_sysfs_tunable

Location: src/tuning/tuningsysfs.cpp : 80–82
Description: `class sysfs_tunable *st;` and `st = new class sysfs_tunable(...)` use the `class` keyword in variable declarations and `new` expressions. In C++ the `class` tag is not required when the class type is already visible; the idiomatic form is simply `sysfs_tunable *st = new sysfs_tunable(...)`. The `class` keyword in declarations is a C-ism that is unnecessary and non-idiomatic in C++.
Severity rationale: Non-idiomatic C++; style inconsistency.
Suggested fix:
```cpp
sysfs_tunable *st = new sysfs_tunable(str, _sysfs_path, _target_content);
all_tunables.push_back(st);
```

## Review of tuningi2c.h, tuningi2c.cpp, powerconsumer.h, powerconsumer.cpp, processdevice.h — batch 07

### Low item #1 : `#ifndef` header guard instead of `#pragma once` in tuningi2c.h

Location: src/tuning/tuningi2c.h : 20-21
Description: The header uses the old `#ifndef _INCLUDE_GUARD_I2C_TUNE_H` idiom instead of the project-standard `#pragma once`.
Severity rationale: Per style.md §4.1, `#pragma once` is the required form.
Suggested fix: Replace lines 20-21 and 44 with `#pragma once`.

### Low item #2 : `#ifndef` header guard instead of `#pragma once` in powerconsumer.h

Location: src/process/powerconsumer.h : 25-26
Description: Same as Low item #1 — old `#ifndef` guard instead of `#pragma once`.
Severity rationale: Per style.md §4.1.
Suggested fix: Replace guard with `#pragma once`.

### Low item #3 : `#ifndef` header guard instead of `#pragma once` in processdevice.h

Location: src/process/processdevice.h : 25-26
Description: Same as Low item #1.
Severity rationale: Per style.md §4.1.
Suggested fix: Replace guard with `#pragma once`.

### Low item #4 : `"unistd.h"` included with quotes instead of angle brackets

Location: src/tuning/tuningi2c.cpp : 22
Description: `#include "unistd.h"` uses double-quotes for a system header. The convention (style.md §4.4) is that system/standard headers use angle brackets `<>`.
Severity rationale: Style violation; can also cause the wrong file to be included if a local `unistd.h` exists.
Suggested fix: `#include <unistd.h>`

### Low item #5 : C-style headers used instead of C++ equivalents

Location: src/tuning/tuningi2c.cpp : 24, 29, 30
Description: `<string.h>`, `<ctype.h>`, and `<limits.h>` are C headers. In C++20 code the C++ wrappers `<cstring>`, `<cctype>`, and `<climits>` should be used so that identifiers are placed in `namespace std`.
Severity rationale: Use of deprecated C-style includes in C++20 code; per general-c++.md, C++20 standard constructs should be preferred.
Suggested fix: Replace with `<cstring>`, `<cctype>`, `<climits>`.

### Low item #6 : `NULL` used instead of `nullptr`

Location: src/process/powerconsumer.cpp : 75, 76
Description: The constructor assigns `waker = NULL` and `last_waker = NULL`. In C++11 and later (and mandated by C++20), `nullptr` is the type-safe null pointer constant.
Severity rationale: `NULL` is a C holdover; `nullptr` is the correct C++20 form per general-c++.md.
Suggested fix: Replace both occurrences with `nullptr`.

### Low item #7 : Silent clamping of inconsistent `child_runtime`

Location: src/process/powerconsumer.cpp : 40-41
Description: When `child_runtime > accumulated_runtime`, the code silently resets `child_runtime = 0` with no log or diagnostic. This masks accounting bugs in callers and makes root-cause analysis harder.
Severity rationale: Defensive silencing of data corruption without any trace; low severity because the clamp prevents a negative cost value, but the lack of any diagnostic makes bugs invisible.
Suggested fix: Add a debug-mode `fprintf(stderr, ...)` or assertion before clamping so that the anomaly is at least visible during development.

## Review of processdevice.cpp, process.h, process.cpp, work.cpp, work.h — batch 08

### Low item #1 : Index-based for loops where range-for is cleaner

Location: src/process/processdevice.cpp : 61 ; src/process/process.cpp : 156, 213
Description: Several loops use `unsigned int i` index variables to iterate over `std::vector` containers (`all_proc_devices`, `all_processes`). C++11 range-for is simpler and eliminates the risk of index type mismatch.
Severity rationale: Style and maintainability; no correctness issue, but inconsistent with modern C++ idioms.
Suggested fix: Replace, e.g.:
```cpp
for (auto *cdev : all_proc_devices) { ... }
for (auto *proc : all_processes)    { ... }
```

### Low item #2 : Verbose iterator type declarations; prefer `auto`

Location: src/process/processdevice.cpp : 93 ; src/process/process.cpp : 221
Description: `std::vector<class device_consumer *>::iterator it = ...` and `std::vector <class process *>::iterator it = ...` are unnecessarily verbose. The `auto` keyword was introduced precisely for this pattern.
Severity rationale: Readability; no correctness issue.
Suggested fix: `auto it = all_proc_devices.begin();` / `auto it = all_processes.begin();`.

### Low item #3 : O(n²) clear_work implementation

Location: src/process/work.cpp : 100-108
Description: `clear_work()` erases one element at a time and resets `it = all_work.begin()` on each iteration, making it O(n²). The standard idiom for clearing a map (freeing each value first) is a simple range-for followed by `all_work.clear()`.
Severity rationale: Performance; for large work-item maps this degrades noticeably.
Suggested fix:
```cpp
void clear_work(void)
{
    for (auto &[key, val] : all_work)
        delete val;
    all_work.clear();
    running_since.clear();
}
```

### Low item #4 : Type inconsistency: constructor takes `unsigned long`, caller passes `uint64_t`

Location: src/process/work.h : 38 ; src/process/work.cpp : 122
Description: `work::work(unsigned long work_func)` accepts `unsigned long`, but `find_create_work(uint64_t func)` passes a `uint64_t`. On LP64 Linux these are the same size, but the inconsistency is confusing and would silently truncate on ILP64 platforms.
Severity rationale: Portability and clarity.
Suggested fix: Change the constructor parameter to `uint64_t`: `work(uint64_t work_func);`.

## Review of interrupt.h, interrupt.cpp, timer.cpp, timer.h, do_process.cpp — batch 09

### Low item #1 : Index-based for loops should be range-based for loops

Location: src/process/interrupt.cpp : 104, 117  /  src/process/do_process.cpp : 107, 117, 720, 733, 747, 762, 776
Description: Numerous loops use the pattern `for (unsigned int i = 0; i < vec.size(); i++)` with index access. C++20 (and the project's general preference for STL constructs) favours range-based for loops which are cleaner and avoid off-by-one errors.
Severity rationale: Style/best-practice violation; no correctness impact but reduces readability.
Suggested fix: Convert to `for (auto *item : vec)` or `for (const auto &item : vec)` as appropriate.

### Low item #2 : all_power.erase(begin, end) should be all_power.clear()

Location: src/process/do_process.cpp : 1144, 1201
Description: `all_power.erase(all_power.begin(), all_power.end())` is equivalent to `all_power.clear()` but is more verbose and less idiomatic C++. `clear()` better expresses intent.
Severity rationale: Readability; no functional difference.
Suggested fix: Replace both occurrences with `all_power.clear();`.

### Low item #3 : add_timer takes pair by value instead of const reference

Location: src/process/timer.cpp : 112
Description: The `add_timer` helper function signature is `static void add_timer(const pair<unsigned long, class timer*>& elem)` — actually the code shows it accepts a `pair` by value. In any case the use with `std::for_each` copies each pair; it should accept `const std::pair<const unsigned long, class timer*>&`.
Severity rationale: Minor inefficiency.
Suggested fix: Use a range-based for loop instead, e.g.:
```cpp
void all_timers_to_all_power(void)
{
    for (auto &[addr, t] : all_timers)
        all_power.push_back(t);
}
```

### Low item #4 : std::string_view wrapping of std::string for starts_with

Location: src/process/do_process.cpp : 284
Description: `!std::string_view(next_comm).starts_with("migration/")` creates an unnecessary `string_view` wrapper. Since `next_comm` is already a `std::string` and C++20 `std::string` has `starts_with()` directly, the cast is redundant.
Severity rationale: Minor inefficiency and unnecessary verbosity.
Suggested fix: Replace with `!next_comm.starts_with("migration/")` etc.

### Low item #5 : Iterator types spelled out instead of using auto

Location: src/process/interrupt.cpp : 124  /  src/process/timer.cpp : 148
Description: `std::vector<class interrupt *>::iterator it = ...` and `std::map<unsigned long, class timer *>::iterator it = ...` are verbose iterator declarations that obscure intent. The project uses C++20; `auto` should be preferred.
Severity rationale: Readability; verbose type declarations.
Suggested fix: Use `auto it = ...`.

### Low item #6 : Local variable timer shadows the class name timer

Location: src/process/timer.cpp : 136
Description: Inside `find_create_timer`, the local variable is named `timer`, which shadows the class name `timer`. While the compiler handles this correctly, it is confusing to read and maintain.
Severity rationale: Readability and potential for confusion.
Suggested fix: Rename to `new_timer` (consistent with `find_create_interrupt` which uses `new_irq`).

## Review of src/perf/perf.h, src/perf/perf_event.h, src/perf/perf_bundle.cpp, src/perf/perf_bundle.h, src/perf/perf.cpp — batch 10

### Low item #1 : `malloc`/`free` used in C++ code instead of `new[]`/`delete[]`

Location: src/perf/perf_bundle.cpp : 63, 91, 160
Description: Raw `malloc` and `free` are used to allocate and release event record buffers. In C++ code, `new[]` / `delete[]` (or `std::vector<unsigned char>`) is strongly preferred. Mixing C and C++ allocation can cause type-safety issues and prevents use of RAII.
Severity rationale: Low — it works but is against C++ best practices and project style.
Suggested fix: Use `new unsigned char[header->size]` / `delete[]` or use a `std::vector<unsigned char>` and store by value.

### Low item #2 : `exit(-1)` instead of `exit(EXIT_FAILURE)`

Location: src/perf/perf.cpp : 113
Description: `exit(-1)` uses a non-portable, non-standard exit code. The portable way to signal failure is `exit(EXIT_FAILURE)`.
Severity rationale: Minor portability issue.
Suggested fix: Replace with `exit(EXIT_FAILURE)`.

### Low item #3 : Unused `eventname` parameter in `create_perf_event`

Location: src/perf/perf.cpp : 58
Description: The `eventname` parameter (of type `const string &`) is marked `__unused` and never referenced inside the function body. The event type is taken from `trace_type` directly. If the parameter is truly unnecessary it should be removed or documented; if it was intended to be used, it is a functional gap.
Severity rationale: Dead parameter increases maintenance burden and could indicate a missing code path.
Suggested fix: Remove the parameter from the signature (updating the call site in `start()`) or document why it is intentionally unused.

### Low item #4 : C-style casts used throughout

Location: src/perf/perf_bundle.cpp : 63, 67, 69, 187, 268; src/perf/perf.cpp : 130, 131, 187, 235
Description: Multiple C-style casts (`(unsigned char *)`, `(struct perf_event_header *)`, `(struct perf_sample *)`, `(perf_event_mmap_page *)`) are used instead of C++ named casts (`reinterpret_cast<>`, `static_cast<>`). C-style casts bypass the type system silently.
Severity rationale: C++ named casts make intent explicit and are safer.
Suggested fix: Replace with appropriate named casts, e.g. `reinterpret_cast<unsigned char *>(...)`.

### Low item #5 : `__unused` macro used instead of C++17 `[[maybe_unused]]`

Location: src/perf/perf.cpp : 58, 280
Description: The project uses `#define __unused __attribute__((unused))` (a GCC extension) rather than the C++17 standard attribute `[[maybe_unused]]`. Since the project targets C++20, the standard attribute should be used.
Severity rationale: Minor — GCC extension works fine but the standard attribute is preferred in C++17/20 code.
Suggested fix: Replace `__unused` with `[[maybe_unused]]` in function parameter declarations.

### Low item #6 : `#include <malloc.h>` is a non-standard header

Location: src/perf/perf_bundle.cpp : 26
Description: `<malloc.h>` is a Linux/glibc-specific header. The standard C/C++ header for `malloc` is `<cstdlib>` (C++) or `<stdlib.h>` (C). `stdlib.h` is already included on line 31 of `perf.cpp`; `perf_bundle.cpp` should use `<cstdlib>` or `<stdlib.h>`.
Severity rationale: Minor portability issue.
Suggested fix: Replace `#include <malloc.h>` with `#include <cstdlib>`.

### Low item #7 : Return value of `fcntl` not checked

Location: src/perf/perf.cpp : 115
Description: `fcntl(perf_fd, F_SETFL, O_NONBLOCK)` is called without checking the return value. If this fails, the fd will remain in blocking mode and subsequent `read`/`process` calls may block indefinitely.
Severity rationale: Silent failure in a system call that affects event processing behavior.
Suggested fix:
```cpp
if (fcntl(perf_fd, F_SETFL, O_NONBLOCK) < 0)
    fprintf(stderr, _("fcntl O_NONBLOCK failed: %s\n"), strerror(errno));
```

## Review of persistent.cpp, parameters.cpp, parameters.h, learn.cpp, report-formatter.h — batch 11

### Low item #1 : `using namespace std` in persistent.cpp

Location: src/parameters/persistent.cpp : 34
Description: `using namespace std;` in a .cpp translation unit is explicitly discouraged by style.md and general-c++.md. While the impact is limited to this translation unit (unlike a header), it is still a rule violation that hides which symbols come from the standard library, reducing readability and increasing collision risk.
Severity rationale: Rule violation contained to one TU.
Suggested fix: Remove the `using namespace std;` line and add the `std::` prefix to `ofstream`, `istringstream`, `string`, `getline`, `setprecision`, `setiosflags`, etc.

### Low item #2 : Old-style iterator and index loops instead of range-based for

Location: src/parameters/persistent.cpp : 50, 55 ; src/parameters/parameters.cpp : 55, 166, 256, 278
Description: Multiple loops use `for (it = …begin(); it != …end(); it++)` or `for (i = 0; i < vec.size(); i++)` patterns. C++20 range-based for (`for (auto &x : container)`) is clearer, less error-prone and preferred by the project style.
Severity rationale: C++20 style violation; no functional impact.
Suggested fix: Convert to range-based for where possible, e.g. `for (auto &[name, idx] : param_index) { … }`.

### Low item #3 : `#ifndef` include guard instead of `#pragma once` in parameters.h

Location: src/parameters/parameters.h : 25
Description: The header uses `#ifndef __INCLUDE_GUARD_PARAMETERS_H_` / `#define` / `#endif` style guards. The project style guide (style.md §4.1) mandates `#pragma once`.
Severity rationale: Style violation; `#pragma once` is universally supported and less error-prone.
Suggested fix: Replace the `#ifndef`/`#define`/`#endif` triple with `#pragma once`.

### Low item #4 : `#ifndef` include guard instead of `#pragma once` in report-formatter.h

Location: src/report/report-formatter.h : 26
Description: Same violation as Low item #3 — `#ifndef _REPORT_FORMATTER_H_` guard instead of `#pragma once`.
Severity rationale: Style violation.
Suggested fix: Replace with `#pragma once`.

### Low item #5 : GCC-specific `__unused` attribute instead of standard `[[maybe_unused]]`

Location: src/report/report-formatter.h : 43, 49, 50, 51, 53, 54
Description: Six virtual method parameters use the GCC extension `__unused` (e.g. `const std::string &str __unused`). C++17 introduced `[[maybe_unused]]` as the portable, standard-compliant spelling. Since the project targets C++20, the standard attribute should be used instead.
Severity rationale: Non-portable, GCC-specific attribute in a C++20 codebase.
Suggested fix: Replace `__unused` with `[[maybe_unused]]` on each affected parameter.

## Review of report-formatter-html.h, report.cpp, report.h, report-formatter-csv.h, report-maker.cpp — batch 12

### Low item #1 : Duplicate `#include <string.h>`

Location: report.cpp : 31, 36
Description: `<string.h>` is included twice. The second inclusion is redundant and suggests copy-paste; the same translation unit also includes `<string>` (the C++ header), so the C header should likely be replaced with `<cstring>` and included only once.
Severity rationale: Harmless (include guards prevent double-definition), but indicates dead code and the wrong header name.
Suggested fix: Remove one of the two occurrences and ensure the remaining one uses `<cstring>`.

### Low item #2 : Non-standard `#include <malloc.h>`

Location: report.cpp : 37
Description: `<malloc.h>` is a GNU extension, not a standard C or C++ header. Nothing in `report.cpp` appears to use any `malloc.h`-specific declarations; `malloc`/`free` are available via `<cstdlib>`. Including it reduces portability.
Severity rationale: Portability issue; the header is unnecessary in this file.
Suggested fix: Remove `#include <malloc.h>`.

### Low item #3 : `NULL` used instead of `nullptr`

Location: report-maker.cpp : 41, report.cpp : 114
Description: `formatter = NULL` (report-maker.cpp:41) and `time(NULL)` (report.cpp:114) use the C-style `NULL` macro. In C++11 and later, `nullptr` is the correct null-pointer constant; `NULL` is implementation-defined and may expand to an integer.
Severity rationale: C++ style / correctness hygiene; not a runtime bug in practice.
Suggested fix: Replace `NULL` with `nullptr` where used as a pointer; `time(nullptr)` for the `time()` call.

### Low item #4 : `formatter` member is a raw owning pointer

Location: report-maker.cpp : 40-51, 99-110
Description: `report_maker` manages the lifetime of its `formatter` through manual `new` / `delete`. This is fragile: if `setup_report_formatter()` is called while an exception propagates (e.g., from `new`), the previously held `formatter` may already have been deleted, leaving the member dangling. Using `std::unique_ptr<report_formatter>` would express ownership clearly and make the destructor body trivial.
Severity rationale: Low — exception propagation from `new` in this code path is very unlikely in practice.
Suggested fix: Declare `formatter` as `std::unique_ptr<report_formatter>` and replace `new`/`delete` with `std::make_unique`.

## Review of report-maker.h, report-data-html.cpp, report-data-html.h, report-formatter-base.cpp, report-formatter-base.h — batch 13

### Low item #1 : C-style headers used instead of C++ equivalents

Location: src/report/report-maker.h : 61  and  src/report/report-formatter-base.cpp : 29, 31
Description: `report-maker.h` includes `<stdarg.h>` and `report-formatter-base.cpp` includes `<stdio.h>` and `<stdarg.h>`. In C++ code these should be `<cstdarg>` and `<cstdio>` respectively, which place their declarations in the `std` namespace and avoid polluting the global namespace with C names.
Severity rationale: C++ best practice; the C headers are deprecated in C++.
Suggested fix: Replace `<stdarg.h>` with `<cstdarg>` and `<stdio.h>` with `<cstdio>`. If neither is actually used in these files, remove the includes entirely.

### Low item #2 : Unqualified `string` in `report-formatter-base.cpp`

Location: src/report/report-formatter-base.cpp : 54, 62
Description: The parameter type in `add(const string &str)` and `add_exact(const string &str)` uses the unqualified name `string` rather than `std::string`. This only compiles because `using namespace std;` is injected transitively via the included headers — a dependency on namespace pollution.
Severity rationale: Style guide violation (style.md §4.2); correct names must be fully qualified.
Suggested fix: Replace `const string &` with `const std::string &` in both definitions.

### Low item #3 : `__()` macro silently depends on a global variable named `report`

Location: src/report/report-maker.h : 68–70
Description: The `__()` translation macro is defined as `((report.get_type() == REPORT_CSV) ? (STRING) : gettext(STRING))`. It silently requires a globally visible `report_maker` object named `report`. Any translation unit that includes `report-maker.h` and uses `__()` without such a global will get a compilation error or, worse, accidentally reference a different `report` object in scope. The macro provides no warning about this hidden dependency.
Severity rationale: Hidden global-state dependency in a header-level macro; fragile and non-obvious to users of the header.
Suggested fix: Rename the macro to something that makes the dependency explicit, or refactor it as a function taking a `const report_maker &` parameter.

## Review of report-formatter-csv.cpp, report-formatter-html.cpp, dram_rapl_device.cpp, dram_rapl_device.h, cpu_linux.cpp — batch 14

### Low item #1 : `dram_rapl_device.h` uses `#ifndef`/`#define` guard instead of `#pragma once`

Location: src/cpu/dram_rapl_device.h : 25-26
Description: The style guide specifies `#pragma once` for header guards. `dram_rapl_device.h` uses the older `#ifndef _INCLUDE_GUARD_DRAM_RAPL_DEVICE_H` / `#define` pattern.
Severity rationale: Deviation from stated style guide. Low impact in practice.
Suggested fix: Replace with `#pragma once` and remove the `#ifndef`/`#define`/`#endif` guards.

### Low item #2 : `NULL` used instead of `nullptr` in C++20 code

Location: src/cpu/dram_rapl_device.h : 46 ; src/cpu/dram_rapl_device.cpp : 40, 50, 57
Description: In C++20 code, `nullptr` is the correct null pointer constant. `NULL` is a C macro and, while still valid, is discouraged in modern C++. In `dram_rapl_device.h` line 46 the default argument uses `NULL`; in `dram_rapl_device.cpp` `time(NULL)` appears three times (though `time()` takes a `time_t*`, so `nullptr` is the right form).
Severity rationale: Style rule violation; `NULL` vs `nullptr` does not affect correctness here but is inconsistent with C++20 practice.
Suggested fix: Replace `NULL` with `nullptr` in all occurrences.

### Low item #3 : `device_name()` and `human_name()` not marked `const` in `dram_rapl_device.h`

Location: src/cpu/dram_rapl_device.h : 48-49
Description: Both `device_name()` and `human_name()` return a compile-time constant string and do not modify any member state, yet neither is declared `const`. The C++ rule states: "Use 'const' when possible for method and function parameters [and methods]."
Severity rationale: Missing `const` prevents calling these methods on const references/pointers and violates the stated coding rule.
Suggested fix:
```cpp
virtual std::string device_name() const { return "DRAM"; }
virtual std::string human_name() const  { return "DRAM"; }
```

### Low item #4 : `cpu_linux.cpp` has duplicate `#include` directives

Location: src/cpu/cpu_linux.cpp : 37-38 and 41, 192
Description: `#include <sys/types.h>` appears on two consecutive lines (37 and 38). Additionally, `#include <format>` appears at both line 41 and again at line 192 (after `measurement_end()`). Duplicate includes are harmless due to include guards but indicate sloppy maintenance.
Severity rationale: Minor style/maintenance issue; no functional impact.
Suggested fix: Remove the duplicate `#include <sys/types.h>` on line 38 and the duplicate `#include <format>` on line 192.

### Low item #5 : Bare `string` type used without `std::` qualifier

Location: src/report/report-formatter-html.cpp : 102, 105, 280, 331 ; src/report/report-formatter-csv.cpp : 52, 55, 112, 143
Description: These files use `string` (and the return type `string` for functions) without the `std::` prefix. The style guide requires always using `std::string`. The bare `string` resolves only because the included headers (`report-formatter-html.h`, `report-formatter-csv.h`) themselves contain `using namespace std;`, which itself is a style violation. The `.cpp` files should use `std::string` explicitly.
Severity rationale: Style rule violation; indirect resolution through a namespace-polluting header.
Suggested fix: Replace all bare `string` with `std::string` in both `.cpp` files, and remove `using namespace std;` from the included headers.

### Low item #6 : C-style headers used instead of C++ equivalents

Location: src/report/report-formatter-html.cpp : 29-31 ; src/report/report-formatter-csv.cpp : 29-31 ; src/cpu/dram_rapl_device.cpp : 25-27 ; src/cpu/cpu_linux.cpp : 32, 35-36
Description: Multiple files include C-style headers (`<stdio.h>`, `<assert.h>`, `<stdarg.h>`, `<stdlib.h>`, `<string.h>`) in C++20 code. The C++ equivalents (`<cstdio>`, `<cassert>`, `<cstdarg>`, `<cstdlib>`, `<cstring>`) are preferred as they place declarations in the `std` namespace and avoid potential macro/symbol pollution.
Severity rationale: Style violation; no functional impact but inconsistent with modern C++20 practice.
Suggested fix: Replace each C-style header with its `<c...>` equivalent.

### Low item #7 : Include order: C standard headers after project headers in `cpu_linux.cpp`

Location: src/cpu/cpu_linux.cpp : 29-40
Description: The style guide specifies "Standard library headers first, followed by project-specific headers." In `cpu_linux.cpp`, `"cpu.h"` (line 29) and `"../lib.h"` (line 30) appear before `<stdlib.h>`, `<unistd.h>`, `<stdio.h>`, `<string.h>`, etc. (lines 32–40).
Severity rationale: Style rule violation.
Suggested fix: Move all standard/system `<...>` headers before the project `"..."` headers.

## Review of src/cpu/cpu_core.cpp, src/cpu/cpudevice.h, src/cpu/cpudevice.cpp, src/cpu/abstract_cpu.cpp, src/cpu/cpu_rapl_device.cpp — batch 15

### Low item #1 : `#ifndef`/`#define` include guard instead of `#pragma once` in cpudevice.h

Location: src/cpu/cpudevice.h : 25-27
Description: The file uses the traditional `#ifndef _INCLUDE_GUARD_CPUDEVICE_H` / `#define`
/ `#endif` pattern. The project style guide (style.md §4.1) mandates `#pragma once`.
Severity rationale: Style guide violation; `#pragma once` is simpler and avoids possible
macro name collisions.
Suggested fix: Replace the three guard lines with `#pragma once`.

### Low item #2 : `#ifndef`/`#define` include guard instead of `#pragma once` in cpu_rapl_device.h

Location: src/cpu/cpu_rapl_device.h : 25-27
Description: Same violation as item #1 in this header.
Severity rationale: Style guide violation; same rationale.
Suggested fix: Replace with `#pragma once`.

### Low item #3 : `NULL` used instead of `nullptr` in cpudevice.h

Location: src/cpu/cpudevice.h : 51
Description: The default parameter `class abstract_cpu *_cpu = NULL` uses the C-style `NULL`
macro. C++11 and later (the project targets C++20) provide the type-safe `nullptr` keyword
which should always be preferred.
Severity rationale: C++20 code should not use the untyped `NULL` macro.
Suggested fix: Change `= NULL` to `= nullptr`.

### Low item #4 : `NULL` used instead of `nullptr` in cpu_rapl_device.h

Location: src/cpu/cpu_rapl_device.h : 46
Description: Same violation — `class abstract_cpu *_cpu = NULL` in the constructor default.
Severity rationale: Same as item #3.
Suggested fix: Change `= NULL` to `= nullptr`.

### Low item #5 : Error diagnostic strings not wrapped in `_()` for translation

Location: src/cpu/abstract_cpu.cpp : 275, 375
Description: The error messages `"Invalid C state finalize "` and `"Invalid P state finalize "`
are not wrapped in the `_()` gettext macro. Even though these are internal diagnostics, the
project rule states all user-visible strings must be translatable. These messages do appear in
the terminal when unexpected state transitions occur, making them user-visible in practice.
Severity rationale: Violates the universal translation requirement (rules.md).
Suggested fix: Wrap the string literals in `_()`, e.g.
`fprintf(stderr, _("Invalid C state finalize %s\n"), linux_name.c_str());`

## Review of src/cpu/cpu_rapl_device.h, src/cpu/cpu_package.cpp, src/cpu/cpu.h, src/cpu/cpu.cpp, src/cpu/intel_cpus.cpp — batch 16

### Low item #1 : cpu_rapl_dev and dram_rapl_dev created with identical packagename

Location: src/cpu/cpu.cpp : 90, 97
Description: Both the `cpu_rapl_device` and `dram_rapl_device` are created using
the same `packagename = pt_format(_("package-{}"), cpu)` string (the assignment
is repeated identically on lines 90 and 97). While this may be intentional, it
means both devices share the same identifier, which could cause confusion in the
device list and in parameter lookups. This looks like a copy-paste artefact.
Severity rationale: Potential functional confusion in device naming/lookup; low risk on current code paths but a maintenance hazard.
Suggested fix: Give the DRAM device a distinct name, e.g. `pt_format(_("dram-package-{}"), cpu)`.

### Low item #2 : `exit(-2)` uses a non-standard exit code

Location: src/cpu/intel_cpus.cpp : 147
Description: `exit(-2)` is called on MSR read failure. Negative exit codes are
non-portable; POSIX only guarantees correct behaviour for 0 and values
representable in the low 8 bits of an unsigned char. On Linux, exit(-2) is
received by the parent as exit code 254 (a confusing, meaningless value).
The project style guide states `exit(EXIT_FAILURE)` for fatal errors.
Severity rationale: Low impact on Linux, but a portability and style issue.
Suggested fix: Replace with `exit(EXIT_FAILURE)`.

### Low item #3 : Inconsistent hex literal case — uppercase `0X`

Location: src/cpu/intel_cpus.cpp : 83, 401
Description: Two hex literals use uppercase `0X8F` and `0X9A` (capital X). Every
other hex literal in these files uses lowercase `0x`. While both forms are valid
C++, the inconsistency is a readability issue and deviates from the established
style.
Severity rationale: Pure style/consistency issue.
Suggested fix: Change to `0x8F` and `0x9A`.

### Low item #4 : Unnecessary `.c_str()` conversion passed to `std::string` parameter

Location: src/cpu/intel_cpus.cpp : 627
Description: `update_pstate(state->freq, state->human_name.c_str(), ...)` converts
`state->human_name` (a `std::string`) to `const char *` for a function that accepts
`const std::string&`. This is an unnecessary round-trip that creates a temporary
`std::string`.
Severity rationale: Minor inefficiency; violates the "prefer std::string over C-style strings" guideline.
Suggested fix: Pass `state->human_name` directly without `.c_str()`.

### Low item #5 : Bare `vector<>` without `std::` in public class members in cpu.h

Location: src/cpu/cpu.h : 103-105
Description: The public data members `children`, `cstates`, and `pstates` in
`abstract_cpu` are declared as bare `vector<...>` without the `std::` prefix:
```cpp
vector<class abstract_cpu *> children;
vector<struct idle_state *> cstates;
vector<struct frequency *> pstates;
```
The project C++ guidelines and style guide both require `std::vector<>`. These
are tolerated at present only because `using namespace std;` is (incorrectly) in
the header, but once that is removed they will not compile.
Severity rationale: Directly tied to the `using namespace std` violation; fixing
that without fixing these would break the build.
Suggested fix: Change to `std::vector<...>` for all three members.

## Review of src/cpu/intel_cpus.h, src/cpu/rapl/rapl_interface.h, src/cpu/rapl/rapl_interface.cpp, src/wakeup/wakeup.h — batch 17

### Low item #1 : `#ifndef` header guard instead of `#pragma once` (intel_cpus.h)

Location: src/cpu/intel_cpus.h : 1
Description: The file uses a traditional `#ifndef`/`#define`/`#endif` header guard instead of the project-mandated `#pragma once` (style.md §4.1).
Severity rationale: Style violation; `#pragma once` is required by the project style guide.
Suggested fix: Replace the `#ifndef` guard with `#pragma once`.

### Low item #2 : `#ifndef` header guard instead of `#pragma once` (rapl_interface.h)

Location: src/cpu/rapl/rapl_interface.h : 24
Description: Same issue as Low item #1 — uses `#ifndef RAPL_INTERFACE_H` instead of `#pragma once`.
Severity rationale: Style violation per style.md §4.1.
Suggested fix: Replace the `#ifndef` guard with `#pragma once`.

### Low item #3 : `#ifndef` header guard instead of `#pragma once` (wakeup.h)

Location: src/wakeup/wakeup.h : 25
Description: Same issue — uses `#ifndef _INCLUDE_GUARD_WAKEUP_H` instead of `#pragma once`.
Severity rationale: Style violation per style.md §4.1.
Suggested fix: Replace the `#ifndef` guard with `#pragma once`.

### Low item #4 : Pervasive typo `measurment_interval` (missing 'e')

Location: src/cpu/rapl/rapl_interface.h : 47, src/cpu/rapl/rapl_interface.cpp : 79, 661, 671, 681, 691, 696
Description: The member variable and all references are spelled `measurment_interval` instead of `measurement_interval`. The typo propagates across the header and all uses in the implementation.
Severity rationale: Misspelling in a protected member variable name; low functional impact but reduces readability and searchability.
Suggested fix: Rename to `measurement_interval` throughout.

### Low item #5 : Missing space between `if` and `(` in multiple locations (rapl_interface.cpp)

Location: src/cpu/rapl/rapl_interface.cpp : 248, 262, 276, 294, 315, 335, 355, 384, 405, 428, 447, 474, 495, 512, 532, 562, 583, 600, 620
Description: Many `if` statements are written as `if(ret < 0)` without a space between the keyword and the opening parenthesis, violating the K&R style requirement (style.md §1.2) and the project's Linux-kernel-variant brace/spacing convention.
Severity rationale: Consistent style violation throughout the file.
Suggested fix: Add a space: `if (ret < 0)`.

### Low item #6 : Space-indented line in intel_cpus.h (mixed indentation)

Location: src/cpu/intel_cpus.h : 56
Description: The line `        DIR *dir;` uses 8 spaces for indentation instead of a tab character. All other members in the class use tab indentation. This breaks the "tabs only" rule from style.md §1.1.
Severity rationale: Mixed indentation in a single class declaration; style violation per style.md §1.1.
Suggested fix: Replace the leading spaces with a tab character.

## Review of wakeup_usb.h, wakeup_ethernet.h, waketab.cpp, wakeup_ethernet.cpp, wakeup.cpp — batch 18

### Low item #1 : duplicate sysfs path construction in wakeup_eth_callback

Location: src/wakeup/wakeup_ethernet.cpp : 99-103
Description: `filename` is set by `std::format(...)` on line 99, checked with `access()`, then set identically again on line 103 before being passed to the constructor. The second assignment is dead code and suggests a copy/paste error. The same pattern occurs in `wakeup_usb_callback`.
Severity rationale: Low — no functional impact, but dead code is misleading and could mask a future bug if lines diverge.
Suggested fix: Remove the second (redundant) `filename = std::format(...)` assignment on line 103.

### Low item #2 : virtual overrides missing `override` specifier

Location: src/wakeup/wakeup_usb.h : 40-44  and  src/wakeup/wakeup_ethernet.h : 40-44
Description: The three virtual methods (`wakeup_value`, `wakeup_toggle`, `wakeup_toggle_script`) declared in both headers are overrides of the base class `wakeup`, but they lack the `override` keyword. Without `override`, a typo in a method signature silently creates a new virtual function instead of overriding the base.
Severity rationale: Low — no current bug, but `override` is a C++11 safety net that costs nothing and prevents subtle errors.
Suggested fix: Add `override` and remove the redundant `virtual`:
```cpp
int wakeup_value(void) override;
void wakeup_toggle(void) override;
std::string wakeup_toggle_script(void) override;
```

### Low item #3 : `#ifndef`/`#define` include guards instead of `#pragma once`

Location: src/wakeup/wakeup_usb.h : 25-26  and  src/wakeup/wakeup_ethernet.h : 25-26
Description: Both headers use old-style `#ifndef _INCLUDE_GUARD_*` guards. The project style guide mandates `#pragma once`.
Severity rationale: Low — style guide violation; `#pragma once` is simpler and universally supported by all toolchains targeted by PowerTOP.
Suggested fix: Replace the `#ifndef`/`#define`/`#endif` guards with `#pragma once`.

### Low item #4 : user-visible string not wrapped in `_()` for translation

Location: src/wakeup/waketab.cpp : 110
Description: `ui_notify_user(std::format(">> {}\n", wakeup_toggle_script))` presents text to the user but the format string `">> {}\n"` is not marked for translation with `_()`. All user-facing strings must use the gettext `_()` macro per the style guide. Additionally, the style guide states user-facing `pt_format` should be used for translated strings with arguments.
Severity rationale: Low — i18n requirement clearly stated in the style guide; the string will never be translated.
Suggested fix:
```cpp
ui_notify_user(pt_format(_(">> {}\n"), wakeup_toggle_script));
```

### Low item #5 : excessive unused includes in wakeup_ethernet.cpp

Location: src/wakeup/wakeup_ethernet.cpp : 27-43
Description: The following headers are included but nothing from them is used in the file: `<sys/socket.h>`, `<errno.h>`, `<linux/types.h>`, `<net/if.h>`, `<linux/sockios.h>`, `<sys/ioctl.h>`, `<linux/ethtool.h>`, `<iostream>`, `<fstream>`, `<stdlib.h>`, `<string.h>`, `<sys/types.h>`, `<unistd.h>`, `<utility>`. These appear to be leftovers from an earlier implementation that used raw ethtool ioctls.
Severity rationale: Low — no functional impact, but unnecessary includes increase compile time, pollute the macro/symbol namespace, and hide the real dependencies of the file.
Suggested fix: Remove all unused includes. Keep only: `<stdio.h>`, `<format>`, `"wakeup.h"`, `"../lib.h"`, `"wakeup_ethernet.h"`.

## Review of src/wakeup/wakeup_usb.cpp, src/measurement/acpi.cpp, src/measurement/acpi.h, src/measurement/opal-sensors.h — batch 19

### Low item #1 : Duplicate `filename` computation in `wakeup_usb_callback`

Location: src/wakeup/wakeup_usb.cpp : 99 and 103
Description: `filename` is computed with `std::format("/sys/bus/usb/devices/{}/power/wakeup", d_name)` on line 99, used only for the `access()` call, and then immediately recomputed with the exact same expression on line 103. The second assignment is dead code — it produces the same value as the first. This is misleading (suggests the value might differ) and wasteful.
Severity rationale: Low — no functional impact, but creates confusion and violates DRY; the double computation also hints at a refactoring left half-done.
Suggested fix: Remove line 103. The existing value of `filename` from line 99 is valid for passing to the `usb_wakeup` constructor.

### Low item #2 : Unused includes in wakeup_usb.cpp

Location: src/wakeup/wakeup_usb.cpp : 33-40
Description: The following headers are included but no symbols from them are referenced anywhere in the file: `<sys/socket.h>`, `<net/if.h>`, `<linux/sockios.h>`, `<sys/ioctl.h>`, `<linux/ethtool.h>`, and `<iostream>`. These appear to be leftover from copy-paste from an ethernet wakeup file. Unnecessary includes increase compilation time and mislead readers about the file's dependencies.
Severity rationale: Low — no functional impact, but violates clean-code principles and harms readability.
Suggested fix: Remove the six unused include directives.

### Low item #3 : Override methods in `acpi_power_meter` lack `override` keyword

Location: src/measurement/acpi.h : 37-42
Description: `start_measurement`, `end_measurement`, `power`, and `dev_capacity` are declared with `virtual` in the derived class `acpi_power_meter` but without the `override` specifier. C++11 and later best practice (and the C++20 standard used by this project) require `override` on overriding functions so the compiler can catch signature mismatches.
Severity rationale: Low — no crash risk but silently hides interface-mismatch bugs; violates C++20 best practices.
Suggested fix: Add `override` and remove the redundant `virtual` on all four overriding method declarations.

### Low item #4 : Override methods in `opal_sensors_power_meter` lack `override` keyword

Location: src/measurement/opal-sensors.h : 34-38
Description: Same issue as batch 19 Low item #3: `start_measurement`, `end_measurement`, `power`, and `dev_capacity` use `virtual` without `override`.
Severity rationale: Low — same rationale as item #3 above.
Suggested fix: Add `override`, remove `virtual`, on all four declarations.

## Review of sysfs.h, sysfs.cpp, measurement.h, measurement.cpp, extech.h — batch 20

### Low item #1 : C headers used instead of C++ headers in sysfs.cpp

Location: src/measurement/sysfs.cpp : 28-30
Description: `#include <string.h>`, `#include <stdio.h>`, and `#include <limits.h>` are C headers. The C++ equivalents (`<cstring>`, `<cstdio>`, `<climits>`) place all symbols in the `std::` namespace, avoiding accidental name collisions. The project uses C++20 throughout.
Severity rationale: Low — compiles correctly, but uses non-idiomatic headers for a C++20 codebase.
Suggested fix: Replace with `<cstring>`, `<cstdio>`, `<climits>`.

### Low item #2 : `power_meters.size() == 0` instead of `power_meters.empty()`

Location: src/measurement/measurement.cpp : 174
Description: Comparing `size()` to 0 is less idiomatic and slightly less efficient than `empty()`. The STL guideline (and general C++ best practice) is to use `empty()` when testing for an empty container.
Severity rationale: Correctness is unaffected, but violates the "prefer STL constructs" rule in general-c++.md.
Suggested fix: `if (power_meters.empty()) {`

### Low item #3 : Index-based loops where range-based for loops should be used

Location: src/measurement/measurement.cpp : 66-67, 73-74, 83-86, 121-125
Description: Four loops iterate over `power_meters` using `unsigned int i` as an index. The project uses C++20 and range-based for loops are cleaner, avoid off-by-one risk, and do not require a manual index variable. general-c++.md states to strongly prefer STL constructs.
Severity rationale: Style / best-practice violation; no functional impact.
Suggested fix: Replace e.g. `for (i = 0; i < power_meters.size(); i++) power_meters[i]->start_measurement();` with `for (auto *m : power_meters) m->start_measurement();`.

### Low item #4 : Unqualified `vector` and unqualified `string` due to namespace pollution

Location: src/measurement/measurement.cpp : 58; src/measurement/sysfs.cpp : 32, 38
Description: `vector<class power_meter *> power_meters;` (measurement.cpp:58) and `const string &` parameters (sysfs.cpp:32,38) are unqualified. They compile only because `using namespace std;` in measurement.h leaks `std::vector` and `std::string`. Once the `using namespace std;` is removed (High item #1), these lines will fail to compile.
Severity rationale: Latent compilation error that will surface when fixing the high-severity issue.
Suggested fix: Use `std::vector<power_meter *>` and `const std::string &` at the affected locations.

## Review of src/measurement/extech.cpp, src/devices/backlight.h, src/devices/rfkill.h, src/devices/rfkill.cpp — batch 21

### Low item #1 : Duplicate includes in extech.cpp

Location: src/measurement/extech.cpp : 37-48 and 58-62
Description: `<string.h>`, `<stdio.h>`, and `<stdlib.h>` are each included twice — once near the top of the file and again after the project headers. While harmless at compile time due to include guards, it indicates copy-paste accumulation and clutters the file.
Severity rationale: Code hygiene; redundant code obscures intent.
Suggested fix: Remove the second block of duplicate includes (lines 60-62).

### Low item #2 : Header guards use `#ifndef` style instead of `#pragma once` (backlight.h, rfkill.h)

Location: src/devices/backlight.h : 25-26 | src/devices/rfkill.h : 25-26
Description: Both headers use the traditional `#ifndef _INCLUDE_GUARD_*` / `#define` / `#endif` pattern. The project style guide (style.md §4.1) mandates `#pragma once`.
Severity rationale: Style rule violation in two files.
Suggested fix: Replace each `#ifndef … #define … #endif` triple with `#pragma once`.

### Low item #3 : Fixed-size `char buf[4096]` in rfkill constructor

Location: src/devices/rfkill.cpp : 47
Description: A fixed-size 4096-byte buffer is used for `readlink()` output. Although the call is correctly bounded with `sizeof(buf) - 1`, the use of a fixed C-style buffer is discouraged by the style guide and general-c++.md in favor of `std::string`-based alternatives. The kernel maximum path length is `PATH_MAX` (typically 4096), so the size is correct, but the idiom is out of place in C++20 code.
Severity rationale: Style/practice concern; provably safe but violates the preference for STL types.
Suggested fix: Use `std::array<char, PATH_MAX>` and reference `sizeof` from that, or use an OS-level path query helper if available.

### Low item #4 : `<unistd.h>` included twice in rfkill.cpp

Location: src/devices/rfkill.cpp : 31 and 42
Description: `<unistd.h>` is included twice in the same translation unit. Harmless but redundant.
Severity rationale: Code cleanliness; signals lack of attention during editing.
Suggested fix: Remove the second `#include <unistd.h>` at line 42.

### Low item #5 : `rate` member initialized in constructor body, not initializer list

Location: src/measurement/extech.cpp : 271
Description: `rate = 0.0;` is assigned in the constructor body instead of the member initializer list. For a plain `double`, there is no performance difference, but it is inconsistent with C++ best practice and with how other data members are initialized.
Severity rationale: Minor C++ best-practice violation.
Suggested fix: Move `rate(0.0)` into the initializer list: `extech_power_meter::extech_power_meter(const string &extech_name) : power_meter(extech_name), rate(0.0)`.

## Review of src/devices/usb.h, src/devices/usb.cpp, src/devices/gpu_rapl_device.h, src/devices/gpu_rapl_device.cpp, src/devices/ahci.h — batch 22

### Low item #1 : Header guards using `#ifndef` instead of `#pragma once`

Location: src/devices/usb.h : 25-26, src/devices/gpu_rapl_device.h : 25-26, src/devices/ahci.h : 25-26
Description: All three headers use the traditional `#ifndef _INCLUDE_GUARD_XXX` / `#define` / `#endif` pattern. The project style guide (`style.md §4.1`) specifies `#pragma once` as the required form. All other recently touched headers in the codebase already use `#pragma once`.
Severity rationale: Style rule violation; non-conforming to stated project standard.
Suggested fix: Replace the `#ifndef`/`#define`/`#endif` triple with a single `#pragma once` at the top of each header (after the copyright block).

### Low item #2 : Include order violation in usb.cpp

Location: src/devices/usb.cpp : 25-39
Description: `#include <iostream>` and `#include <fstream>` (lines 38-39) appear after the project-specific headers `#include "../lib.h"` etc. The project style (`style.md §4.4`) requires standard library headers to appear before project-specific headers. Additionally `#include <iostream>` is never used in this translation unit (no `std::cin`, `std::cout`, etc.), so it is dead code.
Severity rationale: Style violation; dead include adds unnecessary compile-time overhead.
Suggested fix: Move `<iostream>` and `<fstream>` before the project includes, and remove `<iostream>` entirely if it is not needed.

### Low item #3 : C-style `class` keyword in variable declaration and `new` expression

Location: src/devices/usb.cpp : 144, 158
Description: Line 144 declares `class usbdevice *usb;` and line 158 uses `new class usbdevice(...)`. Prefixing the class name with the `class` keyword in expressions is a C idiom not idiomatic C++; in C++ the type name `usbdevice` is sufficient. This is the only file in the recently reviewed batch that does this.
Severity rationale: Style violation; C idiom in C++ code.
Suggested fix: Change to `usbdevice *usb;` and `usb = new usbdevice(...)`.

### Low item #4 : Missing `const` on pure-getter methods

Location: src/devices/usb.h : 55-62, src/devices/gpu_rapl_device.h : 47-50, src/devices/ahci.h : 58-65
Description: Methods such as `class_name()`, `device_name()`, `human_name()`, `power_valid()`, `grouping_prio()`, and `device_present()` access no mutable state and should be declared `const`. The project rules (`general-c++.md`) require `const` where possible. Omitting `const` prevents calling these methods on `const` references or through `const` pointers to the objects.
Severity rationale: C++ best-practice and coding-rule violation; limits const-correctness of callers.
Suggested fix: Append `const` to each getter: e.g., `virtual std::string class_name(void) const { return "usb"; }`.

### Low item #5 : Unused `#include <iostream>` in usb.cpp

Location: src/devices/usb.cpp : 38
Description: `<iostream>` is included but no symbols from it (`std::cin`, `std::cout`, `std::cerr`, etc.) are used anywhere in `usb.cpp`. Dead includes slow compilation and confuse readers.
Severity rationale: Dead code / style violation.
Suggested fix: Remove `#include <iostream>`.

### Low item #6 : Return value of `rapl.get_pp1_energy_status()` is silently ignored

Location: src/devices/gpu_rapl_device.cpp : 39, 47, 57
Description: `c_rapl_interface::get_pp1_energy_status()` presumably returns a status code indicating success or failure. All three call sites discard the return value. If the call fails, `last_energy` / `energy` will hold an outdated or garbage value and `consumed_power` will be computed incorrectly. At minimum, a failure check should log a warning or set `consumed_power = 0.0`.
Severity rationale: Missing error handling; silent failure mode could produce incorrect power readings.
Suggested fix: Check the return value and, on failure, skip the energy update: `if (rapl.get_pp1_energy_status(&energy) == 0) { consumed_power = ...; last_energy = energy; }`.

### Low item #7 : Unused `#include <limits.h>` in usb.h

Location: src/devices/usb.h : 28
Description: `<limits.h>` is included in usb.h but no symbol from it (e.g., `INT_MAX`, `PATH_MAX`) is used in the header. If it was needed in usb.cpp, the include belongs there. Dead includes in headers are transitive and affect all translation units that include usb.h.
Severity rationale: Dead include in a header; increases compile time and can mask missing includes in .cpp files.
Suggested fix: Remove `#include <limits.h>` from usb.h; add it to usb.cpp only if actually needed there (it is not used in usb.cpp either, so remove entirely).


## Review of src/devices/ahci.cpp, src/devices/thinkpad-fan.h, src/devices/thinkpad-fan.cpp, src/devices/device.h, src/devices/device.cpp — batch 23

### Low item #1 : Typo "macine" in user-visible translatable string

Location: src/devices/ahci.cpp : 277
Description: The string `__("AHCI ALPM Residency Statistics - Not supported on this macine")` contains the typo "macine" (should be "machine"). This string appears in the HTML report and is visible to users.
Severity rationale: Typo in a user-visible string; cosmetic but incorrect.
Suggested fix: `__("AHCI ALPM Residency Statistics - Not supported on this machine")`

### Low item #2 : `links.size() == 0` should use `.empty()`

Location: src/devices/ahci.cpp : 276
Description: `if (links.size() == 0)` tests emptiness by comparing the size to zero. The idiomatic and slightly more efficient C++ expression is `links.empty()`.
Severity rationale: Style / best practice; STL guideline prefers `.empty()` for emptiness checks.
Suggested fix: `if (links.empty()) {`

### Low item #3 : `end_devslp` not clamped to `start_devslp` unlike the other three counters

Location: src/devices/ahci.cpp : 156–163
Description: In `ahci::end_measurement`, counter underflow is guarded for `end_active`, `end_partial`, and `end_slumber` (lines 156–162), but `end_devslp` has no corresponding clamp. If the sysfs counter wraps or is reset between `start_measurement` and `end_measurement`, `end_devslp - start_devslp` will be a large negative number. While the subsequent `if (p < 0) p = 0` guard prevents a negative percentage, the unclamped value also distorts `total`, which is used as the divisor for all four percentages.
Severity rationale: Inconsistent defensive logic; can skew all four reported percentages on counter reset/wrap.
Suggested fix: Add `if (end_devslp < start_devslp) end_devslp = start_devslp;` after line 161.

### Low item #4 : Untranslated user-visible strings in thinkpad-fan.h

Location: src/devices/thinkpad-fan.h : 47 and 50
Description: `device_name()` returns `"Fan-1"` and `util_units()` returns `" rpm"`. Both strings may be displayed to the user (device name column, utilisation unit column). Neither is wrapped in `_()`. Per rules.md all user-visible strings must be translatable.
Severity rationale: i18n violation for strings that appear in the device table.
Suggested fix: `return _("Fan-1");` and `return _(" rpm");`

### Low item #5 : Untranslated default `device_name` and `class_name` in device.h

Location: src/devices/device.h : 58–59
Description: The base class `device` returns `"abstract device"` for both `class_name()` and `device_name()`. Should a concrete subclass fail to override these methods, the bare English strings would be displayed to the user without translation.
Severity rationale: Latent i18n risk; currently only triggered if a subclass forgets to override.
Suggested fix: `{ return _("abstract device"); }`

### Low item #6 : Index-based loops should use range-for

Location: src/devices/device.cpp : 107–108, 113–115, 119–122, 187–214, 274–310, 331–334
Description: Multiple loops over `all_devices` use `unsigned int i` index variables and manual `size()` comparisons. Modern C++20 idiom prefers range-for (`for (auto *dev : all_devices) { … }`) which is clearer and avoids potential signed/unsigned comparison warnings.
Severity rationale: Style / best practice; not a functional issue.
Suggested fix: Convert to range-for where the index is not used for anything other than element access.
## Review of src/devices/devfreq.cpp, src/devices/devfreq.h, src/devices/alsa.h, src/devices/alsa.cpp, src/devices/runtime_pm.cpp — batch 24

### Low item #1 : Global `DIR *dir` held open unnecessarily between create and clear

Location: src/devices/devfreq.cpp : 44, 237-253, 325-328
Description: `dir` is opened in `create_all_devfreq_devices()` only to count directory entries and is then left open until `clear_all_devfreq()` is called — potentially hours later. There is no functional reason to keep this handle open; the directory enumeration is done by `process_directory()` which opens the directory independently. Holding the file descriptor wastes a system resource and means the kernel cannot release the dentry.
Severity rationale: Unnecessary resource hold; no correctness benefit.
Suggested fix: Close `dir` with `closedir(dir); dir = NULL;` immediately after the `num == 2` early-exit or right after the counting loop completes; remove the close in `clear_all_devfreq()`.

### Low item #2 : Magic number `7` in `alsa::register_power_with_devlist()`

Location: src/devices/alsa.cpp : 163
Description: `if (name.length() > 7) register_devpower(name.substr(7), ...)` uses the literal `7` to strip a prefix from `name`. The name is formatted as `"alsa:{}"` (prefix length 5). The value 7 corresponds to `"alsa:hw"` (stripping the `alsa:hw` prefix from an ALSA hwCxDy identifier). This is undocumented and error-prone; if the name format ever changes, this silently breaks.
Severity rationale: Unmaintainable magic number; silent breakage risk on format change.
Suggested fix: Define a named constant or use `name.starts_with("alsa:hw") ? name.substr(7) : name` with a clear comment.

### Low item #3 : Duplicate `#include <unistd.h>`

Location: src/devices/alsa.cpp : 30, 41
Description: `<unistd.h>` is included twice. While harmless due to include guards, it is dead code noise.
Severity rationale: Minor code quality issue.
Suggested fix: Remove the second occurrence at line 41.

### Low item #4 : `!all_devfreq.size()` should use `.empty()`

Location: src/devices/devfreq.cpp : 282
Description: `if (!all_devfreq.size())` tests emptiness via the size. The idiomatic and potentially more efficient form is `if (all_devfreq.empty())`.
Severity rationale: Style/best-practice issue; `.empty()` is semantically clearer.
Suggested fix: `if (all_devfreq.empty())`

### Low item #5 : Mixed tabs/spaces indentation in `display_devfreq_devices()`

Location: src/devices/devfreq.cpp : 271-275
Description: Lines 271–275 (`if (!win) return;`, `wclear(win);`, `wmove(win, 2,0);`) use 8 spaces for indentation while the surrounding code uses tabs. style.md §1.1 mandates tabs-only indentation.
Severity rationale: Style violation; inconsistent indentation in a single function.
Suggested fix: Replace the leading 8-space sequences with a single tab character.

### Low item #6 : Mixed tabs/spaces indentation in `runtime_pmdevice::start_measurement()`

Location: src/devices/runtime_pm.cpp : 64
Description: Line 64 (`after_suspended_time = 0;`) uses 8 spaces while the adjacent lines use tabs. style.md §1.1 mandates tabs-only indentation. A similar issue appears at line 97 inside `power_usage()`.
Severity rationale: Style violation.
Suggested fix: Replace the leading spaces with a single tab.
