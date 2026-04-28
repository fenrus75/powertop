# PowerTOP Nit Severity Review Findings

## Review of calibrate.cpp, calibrate.h, lib.cpp, devlist.cpp, main.cpp — batch 01

### Nit item #1 : Mixed `cout` and `printf` in calibrate.cpp

Location: src/calibrate/calibrate.cpp : 389, 412

Description:
`calibrate()` uses `cout << _("...")` at lines 389 and 412 while every other print in the file uses `printf`. The project uses `printf`-style I/O throughout. Mixing the two output mechanisms is a style inconsistency and can cause unexpected ordering on buffered streams.

Suggested fix: Replace the two `cout <<` calls with `printf(_("..."))`.

---

### Nit item #2 : Space indentation instead of tabs in calibrate.cpp

Location: src/calibrate/calibrate.cpp : 387, 415, 419–420

Description:
Several lines inside `calibrate()` use leading spaces instead of the project-mandated tabs (style.md §1.1):
- Line 387: `        save_sysfs(...)` — 8 spaces
- Line 415: `        learn_parameters(...)` — 8 spaces
- Lines 419–420: `        save_all_results(...)` — 8 spaces

Suggested fix: Replace leading spaces with a single tab on each affected line.

---

### Nit item #3 : Space indentation instead of tabs in main.cpp

Location: src/main.cpp : 313–314, 422

Description:
- Lines 313–314 (`fprintf` calls inside `make_report`) use 2-space and 3-space indentation instead of tabs.
- Line 422 (`load_parameters` call inside `powertop_init`) uses 8 spaces instead of a tab.

Suggested fix: Replace leading spaces with tabs to match the rest of the file.

---

### Nit item #4 : Missing space before `!= 0` / `==0` comparisons

Location: src/calibrate/calibrate.cpp : 173; src/devlist.cpp : 88, 164

Description:
```cpp
if (access(filename.c_str(), R_OK)!=0)   // calibrate.cpp:173
if (strncmp(link, "/dev", 4)==0) {        // devlist.cpp:164
```
The project style follows Linux kernel formatting conventions which require spaces around binary operators. These lines are missing spaces around `!=` and `==`.

Suggested fix:
```cpp
if (access(filename.c_str(), R_OK) != 0)
if (strncmp(link, "/dev", 4) == 0) {
```

---

### Nit item #5 : `set_refresh_timeout` returns `0`/`1` instead of `false`/`true`

Location: src/main.cpp : 123–124

Description:
```cpp
if (!time) return 0;
...
return 1;
```
The function signature declares `bool` as the return type, but integer literals `0` and `1` are returned. Prefer `false` and `true` for boolean returns to match the declared type.

Suggested fix:
```cpp
if (!time) return false;
...
return true;
```

## Review of src/test_framework.h, src/test_framework.cpp, src/display.cpp, src/display.h, src/lib.h — batch 02

### Nit item #1 : Spaces instead of tabs in `base64_encode` and `base64_decode`

Location: src/test_framework.cpp : 200–231
Description: Both `base64_encode` and `base64_decode` are indented with 4 spaces instead of the project-mandated tabs. Every line inside these functions is affected.
Severity rationale: Style guide §1.1 mandates tabs only.
Suggested fix: Re-indent with tabs.

---

### Nit item #2 : Spaces instead of tabs in `show_prev_tab()`, `cursor_left()`, `cursor_right()`

Location: src/display.cpp : 192–208, 282–292, 295–306
Description: `show_prev_tab()` and the two cursor functions use spaces for indentation on several lines, inconsistent with the rest of the file and the project style guide.
Severity rationale: Style guide §1.1.
Suggested fix: Convert affected lines to tab indentation.

---

### Nit item #3 : Missing space after `if` in `cursor_up()`

Location: src/display.cpp : 271
Description: `if(w->ypad_pos > 0)` is missing the space between `if` and `(`. The project style (K&R/Linux kernel variant) requires a space.
Severity rationale: Style nit.
Suggested fix: `if (w->ypad_pos > 0)`

---

### Nit item #4 : Missing space around `=` in `tab_window` constructor

Location: src/display.h : 59
Description: `xpad_pos =0;` has a space before `=` but not after. Should be `xpad_pos = 0;` for consistency with the other assignments in the same constructor.
Severity rationale: Style nit.
Suggested fix: `xpad_pos = 0;`

---

### Nit item #5 : Redundant `extern` on function declarations in display.h

Location: src/display.h : 35–47
Description: `extern` on function declarations in a C++ header is implicit and redundant. The project's other headers (e.g., lib.h) also use `extern` inconsistently, but removing it from display.h would align with modern C++ practice.
Severity rationale: Cosmetic / modern C++ idiom.
Suggested fix: Remove `extern` from all function declarations.

---

### Nit item #6 : Signed/unsigned comparison warning in `show_tab()` loop

Location: src/display.cpp : 136
Description: `for (i = 0; i < tab_names.size(); i++)` — `i` is declared as `unsigned int` (matching `tab`'s type) but `tab_names.size()` returns `size_t`. On a 64-bit platform `unsigned int` is 32 bits while `size_t` is 64 bits, producing a comparison between differently-sized unsigned types and potentially a compiler warning.
Severity rationale: Minor; no current runtime risk but indicates imprecision.
Suggested fix: Declare `i` as `size_t` or use a range-for loop.

---

### Nit item #7 : Magic number `7` in `cursor_down()` scroll guard

Location: src/display.cpp : 248
Description: `if ((w->cursor_pos + 7) >= LINES)` uses the literal `7` with no explanation. This appears to be an offset related to the reserved top/bottom display rows, but without a comment or named constant the intent is opaque.
Severity rationale: Readability nit.
Suggested fix: Define `constexpr int DISPLAY_MARGIN = 7;` and document its meaning, or add an inline comment.

## Review of src/tuning/tunable.h, src/tuning/runtime.cpp — batch 03

### Nit item #1 : Extra semicolon after virtual destructor definition

Location: src/tuning/tunable.h : 61
Description: `virtual ~tunable() {};` has a spurious semicolon after the closing brace. While harmless, it is non-idiomatic.
Severity rationale: Cosmetic.
Suggested fix: `virtual ~tunable() {}`

### Nit item #2 : Opening brace on separate line for `for` loop body

Location: src/tuning/runtime.cpp : 174–175
Description: The `for (char blk = 'a'; blk <= 'z'; blk++)` statement has its opening brace on a new line, violating the project's K&R brace style (style.md §1.2).
Severity rationale: Style nit.
Suggested fix: `for (char blk = 'a'; blk <= 'z'; blk++) {`

### Nit item #3 : Inconsistent spacing around `=` in variable declarations

Location: src/tuning/runtime.cpp : 127
Description: `int max_ports = 32, count=0;` — `count=0` lacks spaces around the `=` operator, inconsistent with `max_ports = 32`.
Severity rationale: Cosmetic style inconsistency.
Suggested fix: `int max_ports = 32, count = 0;`

### Nit item #4 : Inconsistent spacing in `for` loop init expression

Location: src/tuning/runtime.cpp : 158
Description: `for (int i=0; i < max_ports; i++)` — `i=0` lacks spaces around `=`.
Severity rationale: Cosmetic style inconsistency.
Suggested fix: `for (int i = 0; i < max_ports; i++)`

### Nit item #5 : Duplicate constant values for `TUNE_UNKNOWN` and `TUNE_NEUTRAL`

Location: src/tuning/tunable.h : 39–40
Description: `TUNE_UNKNOWN` and `TUNE_NEUTRAL` are both defined as `0`. Having two names for the same sentinel makes the codebase ambiguous — it is unclear whether the result of `good_bad()` returning `0` means "unknown state" or "neutral/not applicable".
Severity rationale: Cosmetic / readability issue; could cause confusion when reading code.
Suggested fix: Keep only one name, or add a comment explaining the deliberate dual alias.

### Nit item #6 : C-style `void` in no-argument virtual method declarations

Location: src/tuning/tunable.h : 63, 78, 80, 82
Description: Virtual methods such as `good_bad(void)`, `toggle(void)`, `description(void)`, `toggle_script(void)` and `result_string(void)` use C-style `void` parameter syntax. In C++, empty parentheses `()` are preferred.
Severity rationale: C++ style preference; C-style `(void)` is idiomatic C, not modern C++.
Suggested fix: Change all `method(void)` declarations to `method()`.

### Nit item #7 : Unqualified `string::npos` usage in runtime.cpp relies on namespace pollution

Location: src/tuning/runtime.cpp : 81, 85
Description: `path.find("ata") != string::npos` and `path.find("block") != string::npos` use unqualified `string::npos`. This compiles only because `using namespace std;` is injected via the included `runtime.h`. Once the `using namespace std;` is removed (per High items #1/#2), these must be updated.
Severity rationale: Consequential nit — will become a compile error once the HIGH items are fixed.
Suggested fix: Use `std::string::npos` explicitly.

## Review of ethernet.cpp, ethernet.h, wifi.cpp, wifi.h, tuningusb.h — batch 04

### Nit item #1 : Missing spaces around `<` in comparisons in ethernet.cpp

Location: src/tuning/ethernet.cpp : 71, 78, 107, 114
Description: `sock<0` and `ret<0` are written without spaces around the `<` operator. The project style (K&R/Linux kernel style) requires spaces around binary operators: `sock < 0` and `ret < 0`.
Severity rationale: Minor style inconsistency.
Suggested fix: `if (sock < 0)` and `if (ret < 0)`.

### Nit item #2 : Leading-underscore parameter name `_iface` in wifi_tunable constructor

Location: src/tuning/wifi.cpp : 46
Description: The constructor parameter is named `_iface`. Leading underscores are conventionally reserved for implementation-defined identifiers. The project uses `snake_case` without leading underscores for parameters (e.g., `iface` in ethernet_tunable). Using `_iface` is non-idiomatic and potentially confusing.
Severity rationale: Minor naming convention violation.
Suggested fix: Rename the parameter to `iface` (matching ethernet.cpp's pattern).

### Nit item #3 : Space-indented lines (not tab) on `ioctl` calls in ethernet.cpp

Location: src/tuning/ethernet.cpp : 87, 123, 126
Description: The three `ioctl(sock, SIOCETHTOOL, &ifr);` lines are indented with 8 spaces rather than a tab character. The project style guide mandates tab-only indentation.
Severity rationale: Style guide violation (indentation).
Suggested fix: Replace the leading 8 spaces with a single tab on each of the three lines.

### Nit item #4 : C-style headers used in ethernet.cpp instead of C++ equivalents

Location: src/tuning/ethernet.cpp : 30–32, 37
Description: `<stdio.h>`, `<stdlib.h>`, `<string.h>`, and `<errno.h>` are C-style headers. The C++ equivalents `<cstdio>`, `<cstdlib>`, `<cstring>`, and `<cerrno>` place their declarations in the `std::` namespace and are the idiomatic choice in C++20 code.
Severity rationale: Minor — both forms are standard in practice, but C++ headers are preferred per the C++20 standard and the project's use of C++ idioms elsewhere.
Suggested fix: Replace each C-style header with its C++ equivalent (`<c*>` form).

## Review of tuningusb.cpp, tuning.cpp, tuning.h, bluetooth.cpp, bluetooth.h — batch 05

### Nit item #1 : Bare `string` instead of `std::string`

Location: src/tuning/tuningusb.cpp : 40, 73; src/tuning/tuning.cpp : 247
Description: Several locations use `string` without the `std::` prefix (e.g., `const string &path` in the constructor signature, `string content;` in `good_bad`). This works only because `tunable.h` contains `using namespace std;`, which is itself a style violation. The style guide requires explicit `std::` qualification everywhere.
Severity rationale: Style guide violation; depends on a transitive `using namespace std;`.
Suggested fix: Replace all bare `string` with `std::string` throughout these files.

### Nit item #2 : Redundant `class` keyword in object declaration

Location: src/tuning/tuningusb.cpp : 98
Description: `class usb_tunable *usb;` uses the `class` keyword in a variable declaration. In C++ this is unnecessary (unlike C where `struct` tags require the keyword). The pattern is idiomatic C but not idiomatic C++.
Severity rationale: Style inconsistency; the `class` keyword is superfluous in a variable declaration.
Suggested fix: `usb_tunable *usb;`

### Nit item #3 : Commented-out code block left in production source

Location: src/tuning/bluetooth.cpp : 199–201
Description: A three-line block checking `/sys/module/bluetooth` is commented out with a note explaining the intent. Commented-out code should not remain in production source; either the check should be re-enabled (the concern about triggering autoload is valid) or removed with a commit message explaining the decision.
Severity rationale: Code hygiene; also the original concern (autoloading the bluetooth module) may still be valid.
Suggested fix: If the autoload concern still applies, restore the check. Otherwise remove the dead code entirely.

### Nit item #4 : Redundant explicit `string()` conversion

Location: src/tuning/tuning.cpp : 247
Description: `tunable_data[idx] = string(all_tunables[i]->toggle_script());` — `toggle_script()` already returns `std::string`, so the explicit `string()` constructor call is a no-op copy. It also uses bare `string` instead of `std::string`.
Severity rationale: Unnecessary copy; style violation.
Suggested fix: `tunable_data[idx] = all_tunables[i]->toggle_script();`

### Nit item #5 : Use of POSIX `strcasecmp` instead of a C++ equivalent

Location: src/tuning/tuning.cpp : 177
Description: `strcasecmp(i->description().c_str(), j->description().c_str()) < 0` uses `strcasecmp`, which is a POSIX extension not part of the C++20 standard. The project targets C++20, so a portable alternative should be used.
Severity rationale: Non-standard API; reduces portability.
Suggested fix: Use a locale-aware comparison or a simple case-folding lambda:
```cpp
auto ci_less = [](unsigned char a, unsigned char b){ return std::tolower(a) < std::tolower(b); };
return std::ranges::lexicographical_compare(ia, ib, ci_less);
```
where `ia`/`ib` are the description strings.

### Nit item #6 : Missing space after comma in `wmove` call

Location: src/tuning/tuning.cpp : 110
Description: `wmove(win, 2,0)` — the second comma is not followed by a space, inconsistent with the surrounding code style and with the argument formatting used elsewhere in the file.
Severity rationale: Trivial formatting inconsistency.
Suggested fix: `wmove(win, 2, 0);`

## Review of src/tuning/tuningsysfs.cpp, src/tuning/tuningsysfs.h, src/tuning/nl80211.h, src/tuning/iw.h, src/tuning/iw.c — batch 06

### Nit item #1 : Typo "blatently" in iw.h and iw.c comment headers

Location: src/tuning/iw.h : 2  /  src/tuning/iw.c : 2
Description: Both files contain `"This code has been blatently stolen from"`. The correct spelling is "blatantly".
Severity rationale: Spelling typo in a comment.
Suggested fix: Change "blatently" to "blatantly" in both files.

### Nit item #2 : Missing blank line between last #include and first function definition

Location: src/tuning/tuningsysfs.cpp : 38–39
Description: `#include "../lib.h"` (line 38) is immediately followed by the `sysfs_tunable` constructor definition on line 39 with no separating blank line. Per the project's K&R-style layout convention, a blank line should separate the include block from the first declaration.
Severity rationale: Minor formatting inconsistency.
Suggested fix: Add a blank line between `#include "../lib.h"` and the constructor.

### Nit item #3 : Double-underscore prefix on internal function __handle_cmd

Location: src/tuning/iw.c : 200
Description: The function is named `__handle_cmd`. Per the C and C++ standards, all identifiers beginning with two underscores (`__`) are reserved for the implementation (compiler/standard library). Using such a name in application/library code is technically undefined behaviour and may silently conflict with compiler builtins or future standard library additions.
Severity rationale: Minor standards compliance issue; unlikely to cause practical problems in this codebase.
Suggested fix: Rename to `handle_cmd_internal` or simply `do_handle_cmd`.

## Review of tuningi2c.h, tuningi2c.cpp, powerconsumer.h, powerconsumer.cpp, processdevice.h — batch 07

### Nit item #1 : Redundant `class` keyword in variable declaration and `new` expression

Location: src/tuning/tuningi2c.cpp : 87, 96
Description: `class i2c_tunable *i2c;` and `new class i2c_tunable(...)` use the redundant `class` keyword. In C++ the `class` elaborated-type-specifier is not needed when the type is already declared.
Suggested fix: `i2c_tunable *i2c;` and `new i2c_tunable(...)`.

### Nit item #2 : Redundant forward declaration of `power_consumer`

Location: src/process/powerconsumer.h : 37
Description: `class power_consumer;` is declared on line 37 and then immediately defined starting on line 39. The forward declaration is redundant.
Suggested fix: Remove line 37.

### Nit item #3 : Unused includes in tuningi2c.cpp

Location: src/tuning/tuningi2c.cpp : 27, 28, 29
Description: `<utility>`, `<iostream>`, and `<ctype.h>` are included but no symbols from these headers appear to be used in the file.
Suggested fix: Remove the unused `#include` directives.

### Nit item #4 : Mixed include order in tuningi2c.cpp

Location: src/tuning/tuningi2c.cpp : 20-34
Description: Per style.md §4.4, standard library headers should come first, followed by project-specific headers. Currently project headers (`"tuning.h"`, `"tunable.h"`, `"tuningi2c.h"`) are interleaved with system and C++ headers.
Suggested fix: Reorder into two groups — (1) system/standard headers, (2) project headers — separated by a blank line.

## Review of processdevice.cpp, process.h, process.cpp, work.cpp, work.h — batch 08

### Nit item #1 : Trailing semicolons after closing brace of inline virtual methods

Location: src/process/process.h : 63-64 ; src/process/work.h : 44-47
Description: Inline virtual method definitions end with `};` (e.g., `virtual std::string name(void) { return "process"; };`). The semicolon after `}` is legal but redundant and inconsistent with the rest of the codebase.
Severity rationale: Minor style nit.
Suggested fix: Remove the trailing semicolons.

### Nit item #2 : Unnecessary `class` keyword in `new` expressions and declarations

Location: src/process/processdevice.cpp : 52, 79 ; src/process/process.cpp : 154, 161 ; src/process/work.cpp : 127
Description: Expressions like `new class device_consumer(device)`, `class process *new_proc`, and `class work * work` use the `class` keyword redundantly. In C++ the `class` keyword is not needed when the class name is already in scope.
Severity rationale: Style nit; no functional impact.
Suggested fix: Remove the redundant `class` keyword: `new device_consumer(device)`, `process *new_proc`, etc.

### Nit item #3 : Constructor implementation uses bare `string` instead of `std::string`

Location: src/process/process.cpp : 83
Description: The constructor definition `process::process(const string &_comm, ...)` uses the unqualified `string` while the declaration in `process.h` correctly uses `std::string`. This only compiles due to namespace pollution from `powerconsumer.h`.
Severity rationale: Style inconsistency; should be `const std::string &_comm`.
Suggested fix: Change to `process::process(const std::string &_comm, int _pid, int _tid)`.

### Nit item #4 : `std::for_each` with a named helper function; range-for is more idiomatic

Location: src/process/work.cpp : 89-97
Description: `add_work` is a tiny static helper used only to bridge `std::for_each` into pushing to `all_power`. A range-based for loop is clearer and avoids the extra function:
```cpp
void all_work_to_all_power(void)
{
    for (auto &[key, w] : all_work)
        all_power.push_back(w);
}
```
Severity rationale: Readability nit.

## Review of interrupt.h, interrupt.cpp, timer.cpp, timer.h, do_process.cpp — batch 09

### Nit item #1 : Trailing semicolons after closing braces in inline method definitions

Location: src/process/interrupt.h : 48, 49  /  src/process/timer.h : 46, 47
Description: Inline method definitions end with `};` (a semicolon after the closing brace of the function body). This is valid C++ but the trailing semicolon is redundant and not present elsewhere in the codebase.
Severity rationale: Minor style inconsistency.
Suggested fix: Remove the trailing semicolons: `{ return "interrupt"; }` not `{ return "interrupt"; };`.

### Nit item #2 : Mixed space+tab indentation on line 330

Location: src/process/do_process.cpp : 330
Description: Line 330 (`return;` inside the `sched_wakeup` handler's field check) starts with a space character followed by tabs, inconsistent with the tab-only indentation rule.
Severity rationale: Trivial whitespace inconsistency.
Suggested fix: Replace leading space+tabs with pure tabs.

### Nit item #3 : Double space in static pointer declaration

Location: src/process/do_process.cpp : 50
Description: `static  class perf_bundle * perf_events;` has two consecutive spaces between `static` and `class`.
Severity rationale: Trivial cosmetic issue.
Suggested fix: `static class perf_bundle *perf_events;`

### Nit item #4 : raw_count is int but is only ever incremented

Location: src/process/interrupt.h : 39  /  src/process/timer.h : 36
Description: `raw_count` is declared as `int` in both `interrupt` and `timer`. It is only ever incremented, never assigned a negative value, and compared/used as a count. Using `unsigned int` (or `uint32_t`) would better express the invariant that it is always non-negative.
Severity rationale: Minor type-semantics mismatch.
Suggested fix: Change to `unsigned int raw_count;` in both classes.

## Review of src/perf/perf.h, src/perf/perf_event.h, src/perf/perf_bundle.cpp, src/perf/perf_bundle.h, src/perf/perf.cpp — batch 10

### Nit item #1 : Old-style `#ifndef` include guards instead of `#pragma once`

Location: src/perf/perf.h : 25, src/perf/perf_bundle.h : 25
Description: Both headers use traditional `#ifndef/#define/#endif` include guards. The project style guide specifies `#pragma once`.
Suggested fix: Replace the guard triplet with `#pragma once` at the top of each header.

### Nit item #2 : `#include <errno.h>` duplicated in perf.cpp

Location: src/perf/perf.cpp : 29, 33
Description: `<errno.h>` is included twice. The second inclusion is redundant.
Suggested fix: Remove the duplicate include on line 33.

### Nit item #3 : `int event_added = false;` — wrong type for boolean

Location: src/perf/perf_bundle.cpp : 98
Description: `event_added` is used as a boolean flag but declared as `int` and initialized with `false`. Should be `bool`.
Suggested fix: `bool event_added = false;`

### Nit item #4 : `records.resize(0)` should be `records.clear()`

Location: src/perf/perf_bundle.cpp : 162
Description: `records.resize(0)` is a roundabout way to empty a vector. `records.clear()` is the idiomatic and more readable form.
Suggested fix: `records.clear();`

### Nit item #5 : Double semicolon in `struct trace_entry` declaration

Location: src/perf/perf_bundle.cpp : 171
Description: `} __attribute__((packed));;` — there are two semicolons ending the struct declaration. One is redundant.
Suggested fix: Remove the extra semicolon: `} __attribute__((packed));`

### Nit item #6 : Zero-length array `data[0]` is non-standard C++

Location: src/perf/perf_bundle.cpp : 177
Description: `unsigned char data[0];` is a GCC extension. In standard C99/C11 the flexible array member syntax is `unsigned char data[];`; in C++ flexible array members are not standard at all. Since this is a `__attribute__((packed))` kernel-ABI struct used only for casting, a comment explaining the situation would be helpful.
Suggested fix: At minimum, change to `unsigned char data[];` for C99 conformance; add a comment noting this is a zero-length trailing array for direct pointer arithmetic into the mmap ring buffer.

### Nit item #7 : `#if 0` dead debug code block

Location: src/perf/perf_bundle.cpp : 188
Description: A large `#if 0 ... #endif` block containing `printf` debug statements is left in the source. This should be removed.
Suggested fix: Delete lines 188–212.

### Nit item #8 : `#include <iostream>` in headers where it is unused

Location: src/perf/perf.h : 28, src/perf/perf_bundle.h : 28
Description: Both headers include `<iostream>` but do not use anything from it directly (no `std::cin`, `std::cout`, or `std::cerr` declarations). This header is heavy and slows compilation.
Suggested fix: Remove `#include <iostream>` from both headers. Include it in `.cpp` files that actually use stream I/O.

### Nit item #9 : Missing blank line between consecutive function definitions

Location: src/perf/perf_bundle.cpp : 134
Description: The `start()` and `stop()` function definitions are not separated by a blank line. Similarly `stop()` and `clear()` run together. Consistent blank lines between function definitions improve readability.
Suggested fix: Add a blank line between the closing `}` of `start()` and the opening of `stop()`, and between `stop()` and `clear()`.

### Nit item #10 : Unused `#include <map>` in perf_bundle.h

Location: src/perf/perf_bundle.h : 31
Description: `<map>` is included but `std::map` is not used anywhere in `perf_bundle.h` or `perf_bundle.cpp`.
Suggested fix: Remove the `#include <map>` line.

## Review of persistent.cpp, parameters.cpp, parameters.h, learn.cpp, report-formatter.h — batch 11

### Nit item #1 : Typo "patameters" in `dump_parameter_bundle` declaration

Location: src/parameters/parameters.h : 94
Description: The function declaration reads `void dump_parameter_bundle(struct parameter_bundle *patameters = &all_parameters);` — the parameter name is misspelled "patameters" instead of "parameters".
Severity rationale: Cosmetic typo in a parameter name.
Suggested fix: Rename to `parameters`.

### Nit item #2 : `int` used as boolean for `first` and `bundle_saved` in `load_results`

Location: src/parameters/persistent.cpp : 79, 82
Description: `int first = 1;` and `int bundle_saved = 0;` use plain `int` to represent boolean state. C++ has `bool` for this purpose.
Severity rationale: Minor style issue.
Suggested fix: Declare as `bool first = true;` and `bool bundle_saved = false;`.

### Nit item #3 : Inconsistent indentation inside `if (debug_learning)` block in `learn_parameters`

Location: src/parameters/learn.cpp : 254
Description: The `printf("delta is %5.4f\n", delta);` line at line 254 has an extra level of indentation compared to the surrounding `printf` calls within the same `if` block, breaking the consistent tab-based indentation of the file.
Severity rationale: Pure style nit.
Suggested fix: Align the line with its neighbours inside the `if (debug_learning)` block.

### Nit item #4 : Multiple blocks of commented-out code left in place

Location: src/parameters/learn.cpp : 121–122, 199, 223, 280 ; src/parameters/persistent.cpp : 151
Description: Several blocks of commented-out `printf` / debug statements and a commented-out early-return guard are left in production code. Dead code hurts readability.
Severity rationale: Readability nit.
Suggested fix: Remove all commented-out code. If debug dumps are sometimes needed, guard them with `if (debug_learning)` instead.

## Review of report-formatter-html.h, report.cpp, report.h, report-formatter-csv.h, report-maker.cpp — batch 12

### Nit item #1 : Space indentation instead of tabs on one line

Location: report.cpp : 126
Description: `        init_title_attr(&title_attr);` uses 8 spaces for indentation rather than a single tab. The style guide mandates tab-only indentation throughout.
Severity rationale: Minor, single-line style inconsistency.
Suggested fix: Replace the 8 leading spaces with a single tab character.

### Nit item #2 : Missing braces and misaligned bodies on consecutive `if` statements

Location: report.cpp : 153-155
Description: Three `if` statements share a pattern where the body is on the same line as the condition with no braces and without indentation, e.g.:
```cpp
if (str.length() < 1)
str = read_sysfs_string("/etc/redhat-release");
```
The K&R/Linux style used throughout the project requires braces for all multi-line control-flow blocks, and the body must be indented one level. Also, `str.length() < 1` is better written as `str.empty()`.
Severity rationale: Style violation; the missing indentation could confuse a reader into thinking the body is unconditional.
Suggested fix:
```cpp
if (str.empty())
    str = read_sysfs_string("/etc/redhat-release");
if (str.empty())
    str = read_os_release("/etc/os-release");
```
(Use tabs for indentation.)

## Review of report-maker.h, report-data-html.cpp, report-data-html.h, report-formatter-base.cpp, report-formatter-base.h — batch 13

### Nit item #1 : Inconsistent indentation (spaces instead of tab) in `report-maker.h`

Location: src/report/report-maker.h : 110
Description: The destructor declaration `~report_maker();` is indented with 7 spaces instead of a single tab. Every other declaration in the class is tab-indented. Style guide mandates tabs only (style.md §1.1).
Suggested fix: Replace the leading spaces with a single tab character.

### Nit item #2 : Missing spaces around `=` in assignments in `report-data-html.cpp`

Location: src/report/report-data-html.cpp : 5–6, 23–24, 29–35, and throughout
Description: Many assignments lack spaces around the `=` operator, e.g. `div_attr->css_class=css_class;` and `str= dtmp.str();` (line 123). The project style follows standard K&R/Linux conventions that place spaces around binary operators.
Suggested fix: Add spaces around `=` consistently, e.g. `div_attr->css_class = css_class;`.

### Nit item #3 : `table_size` struct appears to be dead code

Location: src/report/report-data-html.h : 27–30
Description: The `table_size` struct (fields `rows` and `cols`) is declared but not referenced anywhere in the reviewed files or in the wider `src/report/` directory. It duplicates a subset of `table_attributes`.
Suggested fix: Remove the unused `table_size` struct, or document its intended use.

### Nit item #4 : Opening brace on same line as function parameter list in `report-data-html.cpp`

Location: src/report/report-data-html.cpp : 10, 27, 37, 49, 61, 72, 85, 96, 106
Description: Multiple function definitions place `{` on the same line as the closing `)` of the parameter list (e.g., `int rows, int cols){`). The project style guide (style.md §1.2) requires the opening brace of a function definition to be on its own line.
Suggested fix: Move the `{` to the next line for each affected function.

## Review of report-formatter-csv.cpp, report-formatter-html.cpp, dram_rapl_device.cpp, dram_rapl_device.h, cpu_linux.cpp — batch 14

### Nit item #1 : Empty `init_markup()` body with placeholder comment

Location: src/report/report-formatter-html.cpp : 61-63
Description: `init_markup()` contains only the comment `/*here all html code*/` and is otherwise empty. It is called from the constructor but does nothing. This is dead placeholder code that adds noise without value.
Severity rationale: Code quality nit; no functional impact.
Suggested fix: Either populate the function or remove it and remove the call from the constructor.

### Nit item #2 : C-style `(void)` parameter syntax in C++ code

Location: src/cpu/dram_rapl_device.h : 52-53
Description: `start_measurement(void)` and `end_measurement(void)` use the C-style explicit `void` parameter list. In C++, empty parentheses `()` are idiomatic and mean the same thing. This is a style inconsistency.
Severity rationale: Purely cosmetic; no semantic difference in C++.
Suggested fix:
```cpp
void start_measurement();
void end_measurement();
```

### Nit item #3 : Trailing semicolons on method bodies in `dram_rapl_device.h`

Location: src/cpu/dram_rapl_device.h : 48-49
Description: The inline method definitions `{return "DRAM";}` are followed by an extra `;` after the closing brace (e.g., `virtual std::string device_name(void) {return "DRAM";};`). While harmless in C++, the trailing semicolons are unnecessary and inconsistent with the rest of the codebase.
Severity rationale: Cosmetic nit.
Suggested fix: Remove the trailing `;` after each method body's closing `}`.

## Review of src/cpu/cpu_core.cpp, src/cpu/cpudevice.h, src/cpu/cpudevice.cpp, src/cpu/abstract_cpu.cpp, src/cpu/cpu_rapl_device.cpp — batch 15

### Nit item #1 : Double semicolons `;;` in cpudevice.cpp constructor

Location: src/cpu/cpudevice.cpp : 39-42
Description: Each of the four index-initialisation lines ends with `;;` (double semicolons):
e.g. `wake_index = get_param_index("cpu-wakeups");;`. This is a harmless no-op (an empty
statement), but is clearly a typo and looks sloppy.
Suggested fix: Remove the extra semicolons so each line ends with a single `;`.

### Nit item #2 : Missing space around `==` operator in cpu_core.cpp

Location: src/cpu/cpu_core.cpp : 78
Description: `if (total_stamp ==0)` is missing a space before `==`. The project style (K&R /
Linux kernel variant) requires spaces around binary operators.
Suggested fix: Change to `if (total_stamp == 0)`.

### Nit item #3 : `last_stamp = 0` assigned twice in `measurement_start`

Location: src/cpu/abstract_cpu.cpp : 98, 127
Description: `last_stamp = 0;` appears on line 98 and again on line 127 in the same function
body with no intervening code that could change it. The second assignment is redundant.
Suggested fix: Remove the duplicate assignment on line 127.

### Nit item #4 : Unqualified `string` used in function signatures

Location: src/cpu/cpu_core.cpp : 33; src/cpu/abstract_cpu.cpp : 262, 284, 343, 383
Description: Several function signatures use unqualified `string` (e.g.
`const string &linux_name`) rather than `std::string`. This only compiles because `cpu.h`
contains `using namespace std;` (a separate violation). Style.md §4.3 and general-c++.md
both require the `std::` prefix.
Suggested fix: Use `const std::string &` consistently in all signatures.

### Nit item #5 : Redundant loop-variable initialisation in destructor

Location: src/cpu/abstract_cpu.cpp : 37-38
Description: The destructor declares `unsigned int i=0;` and then immediately re-initialises
it in the `for` statement (`for (i=0; ...)`). The separate declaration with `=0` is redundant.
Suggested fix: Declare `i` inside the `for` statement: `for (unsigned int i = 0; i < cstates.size(); i++)`.

## Review of src/cpu/cpu_rapl_device.h, src/cpu/cpu_package.cpp, src/cpu/cpu.h, src/cpu/cpu.cpp, src/cpu/intel_cpus.cpp — batch 16

### Nit item #1 : Missing spaces around operators and after commas

Location: src/cpu/cpu_package.cpp : 88; src/cpu/cpu.cpp : 500, 708, 837, 466-468
Description: Several places have missing spaces around operators and assignment:
- `total_stamp ==0` (cpu_package.cpp:88, also intel_cpus.cpp:352, 484, 712)
- `cpu_tbl_size.cols=(2 * ...` — no space around `=` (cpu.cpp:500)
- `wmove(win, 2,0)` — no space after comma (cpu.cpp:837)
- `idx1=0; idx2=0; idx3=0;` — no spaces around `=` (cpu.cpp:466-468)
The project style uses spaces around operators consistently throughout other files.
Severity rationale: Pure cosmetic style issue.
Suggested fix: Add spaces to match project style: `total_stamp == 0`, `idx1 = 0`, etc.

### Nit item #2 : Bare `string` without `std::` in .cpp files

Location: src/cpu/cpu.cpp : 462, 661; src/cpu/cpu_package.cpp : 43
Description: `string tmp_str;` (cpu.cpp:462, 661) and `const string &separator`
(cpu_package.cpp:43) use unqualified `string` instead of `std::string`. These work
only because of the `using namespace std;` in the included headers; once that is
corrected these will need to be qualified.
Severity rationale: Style issue that will become a compile error once `using namespace std` is removed from headers.
Suggested fix: Use `std::string` consistently.

### Nit item #3 : Mixed indentation (spaces instead of tabs) in cpu.cpp

Location: src/cpu/cpu.cpp : 461, 656, 837
Description: Several lines use leading spaces instead of tabs for indentation
(e.g. `       \tint idx1, idx2, idx3;` at line 461, `        report.add_div(...)` at line
656, `        wmove(win, 2,0)` at line 837). The project style mandates tabs only.
Severity rationale: Pure cosmetic style issue.
Suggested fix: Convert leading spaces to tabs in these lines.

## Review of src/cpu/intel_cpus.h, src/cpu/intel_gpu.cpp, src/cpu/rapl/rapl_interface.cpp, src/wakeup/wakeup.h — batch 17

### Nit item #1 : Trailing semicolon after inline method closing brace

Location: src/cpu/intel_cpus.h : 86, 109, 128
Description: Three inline method definitions end with `{ return 0;};` — the trailing semicolon after `}` is redundant. Example: `virtual int can_collapse(void) { return 0;};`. In standard C++ the semicolon after a function body's `}` is only required for class/struct definitions, not for member function definitions.
Severity rationale: Minor style noise; superfluous punctuation.
Suggested fix: Remove the trailing semicolons: `virtual int can_collapse(void) { return 0; }`.

### Nit item #2 : Typo in MSR define names — `ENERY` instead of `ENERGY`

Location: src/cpu/rapl/rapl_interface.cpp : 50, 55, 60, 66
Description: Four `#define` constants are misspelled: `MSR_PKG_ENERY_STATUS`, `MSR_DRAM_ENERY_STATUS`, `MSR_PP0_ENERY_STATUS`, `MSR_PP1_ENERY_STATUS`. The word "ENERGY" is missing its second 'G'.
Severity rationale: Cosmetic typo in macro names; no functional impact since the names are used consistently.
Suggested fix: Rename all four macros to use `ENERGY` and update all references.

### Nit item #3 : Trailing whitespace in intel_gpu.cpp switch statement

Location: src/cpu/intel_gpu.cpp : 68
Description: The line `switch (line_nr) {` is followed by a trailing tab character, violating the "no trailing whitespace" rule from style.md §1.1.
Severity rationale: Trailing whitespace style violation.
Suggested fix: Remove the trailing tab.

### Nit item #4 : Missing space in `#include<vector>` directive

Location: src/wakeup/wakeup.h : 28
Description: The include directive is written as `#include<vector>` without the conventional space between `#include` and the angle-bracket filename. All other includes in the project and file use `#include <...>`.
Severity rationale: Minor inconsistency in formatting.
Suggested fix: Change to `#include <vector>`.

## Review of wakeup_usb.h, wakeup_ethernet.h, waketab.cpp, wakeup_ethernet.cpp, wakeup.cpp — batch 18

### Nit item #1 : missing spaces around `=` and before `{` in report_show_wakeup

Location: src/wakeup/waketab.cpp : 141-142
Description: Line 141 has `if (rows > 0){` (missing space before `{`), and line 142 has `rows= rows + 1;` (missing space after `=`). Both are style violations per the K&R/Linux coding style used by the project.
Severity rationale: Nit — purely cosmetic, but inconsistent with the surrounding code.
Suggested fix:
```cpp
if (rows > 0) {
    rows = rows + 1;
```

### Nit item #2 : redundant `string(...)` cast in report_show_wakeup

Location: src/wakeup/waketab.cpp : 158
Description: `wakeup_data[idx] = string(wakeup_all[i]->wakeup_toggle_script())` — the explicit `string()` constructor call is redundant because `wakeup_toggle_script()` already returns `std::string`, and the assignment operator would copy-assign it directly.
Severity rationale: Nit — no functional issue; just unnecessary noise.
Suggested fix:
```cpp
wakeup_data[idx] = wakeup_all[i]->wakeup_toggle_script();
```

### Nit item #3 : inconsistent loop variable names (tgb vs gb) in report_show_wakeup

Location: src/wakeup/waketab.cpp : 133, 153
Description: The first loop uses `int tgb` and the second loop uses `int gb` for the same logical value (result of `wakeup_value()`). The inconsistency makes the relationship between the two loops harder to see and suggests the code was written in separate passes.
Severity rationale: Nit — no functional impact; purely a readability issue.
Suggested fix: Use a consistent name, e.g., `val`, in both loops.

### Nit item #4 : missing space after comma in wmove call

Location: src/wakeup/waketab.cpp : 69
Description: `wmove(win, 1,0)` is missing a space after the second comma. The rest of the codebase consistently puts a space after each comma in argument lists.
Severity rationale: Nit — cosmetic style inconsistency.
Suggested fix: `wmove(win, 1, 0);`

## Review of src/wakeup/wakeup_usb.cpp, src/measurement/acpi.cpp, src/measurement/acpi.h, src/measurement/opal-sensors.cpp, src/measurement/opal-sensors.h — batch 19

### Nit item #1 : Redundant `class` keyword in `new` expression

Location: src/wakeup/wakeup_usb.cpp : 104
Description: `usb = new class usb_wakeup(filename, d_name);` — the `class` keyword before the type name in a `new` expression is valid C++ but entirely redundant and unusual. This is a C-ism not needed in C++.
Severity rationale: Nit — purely stylistic, no functional impact.
Suggested fix: `usb = new usb_wakeup(filename, d_name);`

### Nit item #2 : GCC `__unused` attribute instead of C++17 `[[maybe_unused]]`

Location: src/wakeup/wakeup_usb.cpp : 48
Description: The `path` constructor parameter uses the GCC-specific `__unused` macro instead of the standard C++17 (and C++20) attribute `[[maybe_unused]]`. The project targets C++20 and should use standard attributes.
Severity rationale: Nit — non-portable; `[[maybe_unused]]` is the correct C++ idiom.
Suggested fix: `usb_wakeup::usb_wakeup(const std::string &path [[maybe_unused]], const std::string &iface)`

### Nit item #3 : Unprofessional comment in `acpi_power_meter::measure()`

Location: src/measurement/acpi.cpp : 111
Description: The comment reads `/* BIOS report random crack-inspired units. Lets try to get to the Si-system units */`. The phrase "crack-inspired" is unprofessional and inappropriate in a project codebase. Minor grammar issue: "Lets" should be "Let's".
Severity rationale: Nit — code quality and professionalism.
Suggested fix: Replace with: `/* BIOS firmware may report non-SI units; normalise to SI. */`

### Nit item #4 : Old-style `#ifndef` header guard in acpi.h

Location: src/measurement/acpi.h : 25-26
Description: Uses `#ifndef __INCLUDE_GUARD_ACPI_H` / `#define __INCLUDE_GUARD_ACPI_H` instead of `#pragma once` as required by the project style guide (style.md §4.1).
Severity rationale: Nit — direct style-guide violation; `#pragma once` is simpler and universally supported by all targeted compilers.
Suggested fix: Replace lines 25-26 and 44 with a single `#pragma once`.

### Nit item #5 : Old-style `#ifndef` header guard in opal-sensors.h

Location: src/measurement/opal-sensors.h : 25-26
Description: Same as Nit item #4 above — uses `#ifndef INCLUDE_GUARD_OPAL_SENSORS_H` instead of `#pragma once`.
Severity rationale: Nit — same rationale.
Suggested fix: Replace with `#pragma once`.

### Nit item #6 : Trailing semicolon after empty function bodies in opal-sensors.h

Location: src/measurement/opal-sensors.h : 34-35
Description: `virtual void start_measurement(void) {};` and `virtual void end_measurement(void) {};` have a trailing `;` after the closing brace. While syntactically valid (it is an empty statement), this is not idiomatic C++ and is not used consistently elsewhere.
Severity rationale: Nit — minor style inconsistency.
Suggested fix: Remove the trailing `;` from both lines.

### Nit item #7 : Unnecessary `#include <iostream>` in acpi.cpp

Location: src/measurement/acpi.cpp : 27
Description: `<iostream>` is included but no `std::cin`, `std::cout`, `std::cerr`, or any other iostream symbol is used anywhere in the file. This is a dead include.
Severity rationale: Nit — dead include, minor readability issue.
Suggested fix: Remove `#include <iostream>`.

## Review of sysfs.h, sysfs.cpp, measurement.h, measurement.cpp, extech.h — batch 20

### Nit item #1 : Redundant `this->` in sysfs_power_meter::measure

Location: src/measurement/sysfs.cpp : 133, 139
Description: `this->set_discharging(false)` and `this->set_discharging(true)` use an explicit `this->` that is unnecessary in non-template context. The calls are unambiguous without it.
Severity rationale: Purely cosmetic; no functional impact.
Suggested fix: Remove `this->` and write `set_discharging(false);` / `set_discharging(true);`.

### Nit item #2 : Redundant default-initialization of `name` in power_meter default constructor

Location: src/measurement/measurement.h : 38
Description: `power_meter() : name("") {}` explicitly initializes `std::string name` to an empty string. `std::string` already default-initializes to `""`, so the explicit initializer is redundant.
Severity rationale: Cosmetic/clarity.
Suggested fix: `power_meter() = default;` or simply `power_meter() {}`.

### Nit item #3 : Stray semicolons after closing brace `}` in class bodies

Location: src/measurement/measurement.h : 39; src/measurement/extech.h : 47
Description: Both `virtual ~power_meter() {};` and `virtual double dev_capacity(void) { return 0.0; };` have a trailing semicolon after the closing `}` of the inline body. The semicolon is syntactically harmless (empty declaration) but is not idiomatic and may confuse readers.
Severity rationale: Cosmetic.
Suggested fix: Remove the trailing semicolons.

### Nit item #4 : `new class extech_power_meter` uses C-style `class` keyword

Location: src/measurement/measurement.cpp : 183
Description: `new class extech_power_meter(devnode)` prefixes the type name with `class`, which is a C-style elaborated type specifier. In idiomatic C++ the `class` keyword is omitted in expressions.
Severity rationale: Cosmetic; compiles correctly but is non-idiomatic.
Suggested fix: `new extech_power_meter(devnode)` (also apply `std::nothrow` per Medium item #5).

### Nit item #5 : Local variable names shadow virtual method names in sysfs.cpp

Location: src/measurement/sysfs.cpp : 72, 86, 102, 115
Description: Local variable names `power`, `current`, `energy`, and `charge` shadow the inherited virtual method `power_meter::power()`. The code compiles correctly because C++ disambiguates method calls from variables, but the shadowing makes the code harder to read at a glance.
Severity rationale: Readability; no functional impact.
Suggested fix: Rename the locals to e.g. `power_uw`, `current_ua`, `energy_uwh`, `charge_uah` to reflect their units and avoid shadowing.

## Review of src/measurement/extech.cpp, src/devices/backlight.cpp — batch 21

### Nit item #1 : C-style cast and `return 0` instead of `nullptr` in thread_proc

Location: src/measurement/extech.cpp : 321-323
Description: `thread_proc` uses a C-style cast `(class extech_power_meter*)arg` instead of `static_cast<extech_power_meter*>(arg)`, and returns `0` instead of `nullptr` for a `void *` return type.
Severity rationale: Style nit; C-style casts are discouraged in C++20.
Suggested fix: `return static_cast<extech_power_meter*>(arg);` pattern plus `return nullptr;`.

### Nit item #2 : `&p.buf` passed to read() instead of `p.buf`

Location: src/measurement/extech.cpp : 258
Description: `read(fd, &p.buf, 250)` takes the address of the array `p.buf`, yielding a `char (*)[256]` rather than a `char *`. Both decay to the same pointer value, so it works, but it is technically taking the address of the array object, not a pointer to the first element.
Severity rationale: Cosmetic; no practical impact due to pointer equivalence.
Suggested fix: Change to `read(fd, p.buf, 250)`.

### Nit item #3 : `bl_boost_index80` and `bl_boost_index100` not explicitly zero-initialized

Location: src/devices/backlight.cpp : 143
Description: The static local variables `bl_boost_index80` and `bl_boost_index100` are declared without an explicit initializer in the same declaration that zero-initializes the other four variables. They are implicitly zero-initialized because they are `static`, but the omission is inconsistent and confusing.
Severity rationale: Cosmetic inconsistency.
Suggested fix: Add `= 0` to both: `static int bl_index = 0, blp_index = 0, bl_boost_index40 = 0, bl_boost_index80 = 0, bl_boost_index100 = 0;`.

### Nit item #4 : Extra space before `<` operator in rfkill::utilization()

Location: src/devices/rfkill.cpp : 97
Description: `if (rfk <  start_hard+end_hard)` has two spaces before `start_hard`, and no spaces around the `+` operators. Inconsistent spacing.
Severity rationale: Formatting nit.
Suggested fix: `if (rfk < start_hard + end_hard)`.

### Nit item #5 : `for (i = 0; i < 1; i++)` is an unnecessary loop for a single iteration

Location: src/measurement/extech.cpp : 222
Description: A `for` loop that always executes exactly once (bound of `< 1`) adds no value and obscures the intent. Either the bound is wrong (see Medium item #3) or the loop should be removed.
Severity rationale: Clarity nit.
Suggested fix: If only one iteration is intended, remove the loop and use `i = 0` directly. If four iterations are intended, fix the bound as noted in Medium item #3.

## Review of src/devices/usb.h, src/devices/usb.cpp, src/devices/gpu_rapl_device.h, src/devices/gpu_rapl_device.cpp, src/devices/ahci.h — batch 22

### Nit item #1 : Spaces instead of tabs on usb.cpp line 120

Location: src/devices/usb.cpp : 120
Description: The line `        register_devpower(devfs_name, power_usage(results, bundle), this);` uses 8 spaces for indentation instead of a tab character. Every other line in the file uses tabs. The project style (`style.md §1.1`) mandates tabs only.
Severity rationale: Style nit; single inconsistent indentation character.
Suggested fix: Replace the leading 8 spaces with a single tab.

### Nit item #2 : C-style `(void)` parameter notation in method declarations

Location: src/devices/usb.h : 50-61, src/devices/ahci.h : 53-56, src/devices/gpu_rapl_device.h : 47-53
Description: Methods are declared with `(void)` to indicate no parameters (e.g., `virtual void start_measurement(void);`). In C++ the idiomatic form is empty parentheses `()`. The `(void)` form is a C90 convention to distinguish "no parameters" from "unspecified parameters" — a distinction that does not apply in C++.
Severity rationale: Style nit; C idiom in C++ code.
Suggested fix: Change all `(void)` parameter lists to `()` in method declarations.

### Nit item #3 : Extra blank line inside usb.cpp constructor body

Location: src/devices/usb.cpp : 63
Description: There is a blank line between the `cached_valid = 0;` statement and the `/* root ports and hubs ... */` comment inside the constructor. While a single blank line for visual grouping is fine, the line at 63 is entirely redundant as line 62 already provides separation. Minor visual noise.
Severity rationale: Style nit.
Suggested fix: Remove the extra blank line.


## Review of src/devices/ahci.cpp, src/devices/thinkpad-fan.h, src/devices/thinkpad-fan.cpp, src/devices/device.h, src/devices/device.cpp — batch 23

### Nit item #1 : Space indentation instead of tabs in `report_devices`

Location: src/devices/device.cpp : 155–156, 160–161
Description: Lines `if (!win)`, `return;`, `wclear(win);`, and `wmove(win, 2,0);` are indented with 8 spaces instead of a tab. style.md §1.1 requires "Tabs only" for indentation.
Severity rationale: Style inconsistency; does not affect behaviour.
Suggested fix: Replace the leading spaces on those lines with a single tab character.

### Nit item #2 : Missing spaces around `==` operator in character comparisons

Location: src/devices/ahci.cpp : 61, 91
Description: `d_name[0]=='.'` (line 61 and similarly line 91) omits spaces around the `==` operator. The project consistently uses spaces around binary operators elsewhere.
Severity rationale: Minor style inconsistency.
Suggested fix: `d_name[0] == '.'`

### Nit item #3 : Missing space before `{` in `for` loop

Location: src/devices/ahci.cpp : 299
Description: `for (i = 0; i < links.size(); i++){` — the opening brace immediately follows `)` with no space. style.md §1.2 (K&R style) requires a space before `{`.
Severity rationale: Style inconsistency.
Suggested fix: `for (i = 0; i < links.size(); i++) {`

### Nit item #4 : Extra semicolons after inline method definition closing braces in thinkpad-fan.h

Location: src/devices/thinkpad-fan.h : 45–52
Description: Inline methods such as `virtual std::string class_name(void) { return "fan";};` have a redundant `;` after the closing `}`. The `;` is not illegal (it is an empty declaration) but is unnecessary and inconsistent with common style.
Severity rationale: Cosmetic; harmless.
Suggested fix: Remove the trailing `;` after each inline method body's `}`.

### Nit item #5 : String parameter `shortname` marked `__unused` but passed to recursive helper

Location: src/devices/ahci.cpp : 47
Description: `disk_name` accepts `const string &shortname __unused`, marking the parameter as unused. However, the function does use `shortname` conceptually (it was passed for fallback use); the `__unused` attribute suppresses the warning but also obscures intent. More precisely, the variable is genuinely unused inside `disk_name` — it was passed in but the code never reads it. Consider removing the parameter entirely and updating the call site in `model_name` at line 98 (`disk_name(pathname, d_name, shortname)` → `disk_name(pathname, d_name)`).
Severity rationale: Interface clarity nit; unused parameter that is silently ignored.
Suggested fix: Remove `shortname` from the `disk_name` signature and its call site.
## Review of src/devices/devfreq.cpp, src/devices/devfreq.h, src/devices/alsa.h, src/devices/alsa.cpp, src/devices/runtime_pm.cpp — batch 24

### Nit item #1 : Trailing semicolons after closing braces on inline virtual functions in devfreq.h

Location: src/devices/devfreq.h : 59-65
Description: Inline virtual method definitions end with `};` (semicolon after the closing brace), e.g. `virtual std::string class_name(void) { return "devfreq";};`. The trailing semicolon is not required and inconsistent with the rest of the codebase.
Severity rationale: Minor style nit; not a correctness issue.
Suggested fix: Remove the trailing semicolons: `virtual std::string class_name(void) { return "devfreq"; }`

### Nit item #2 : Missing spaces around operators in `for` loop in devfreq.cpp

Location: src/devices/devfreq.cpp : 76, 108, 164, 213, 220, 287, 292, 315, 318
Description: Several `for` loops use compact forms like `for (i=0; i<all_devfreq.size(); i++)` without spaces around `=` and `<`. The project style follows Linux kernel conventions which use `i = 0`, `i < n`, `i++` with spaces.
Severity rationale: Cosmetic style inconsistency.
Suggested fix: `for (i = 0; i < all_devfreq.size(); i++)`

### Nit item #3 : `parse_devfreq_trans_stat()` parameter `dname` is unused

Location: src/devices/devfreq.cpp : 121
Description: The function signature is `void devfreq::parse_devfreq_trans_stat(const string &dname __unused)` but the function always uses the member variable `dir_name` instead of the parameter. The `__unused` annotation confirms the parameter is never read. The parameter should either be removed or the function refactored to use it.
Severity rationale: Dead parameter causes confusion about the function's interface.
Suggested fix: Remove the `dname` parameter and update the declaration in devfreq.h accordingly; the function uses `dir_name` exclusively.

### Nit item #4 : `update_devfreq_freq_state()` does not break early on match

Location: src/devices/devfreq.cpp : 108-111
Description: The loop in `update_devfreq_freq_state()` scans the entire `dstates` vector even after finding the matching frequency. A `break` after the assignment on line 110 would avoid unnecessary iterations.
Severity rationale: Minor inefficiency; no correctness impact.
Suggested fix: Add `break;` after `state = dstates[i];` inside the `if` branch.

## Review of runtime_pm.h, network.cpp, network.h, i915-gpu.h, i915-gpu.cpp — batch 25

### Nit item #1 : Old-style `#ifndef` include guards instead of `#pragma once`

Location: src/devices/runtime_pm.h : 25–26; src/devices/network.h : 25–26; src/devices/i915-gpu.h : 25–26
Description: All three header files use `#ifndef _INCLUDE_GUARD_…` / `#define` / `#endif` include guards. The project style guide (style.md §4.1) specifies `#pragma once`.
Suggested fix: Replace the `#ifndef`/`#define`/`#endif` triple with `#pragma once` at the top of each header.

### Nit item #2 : C header `<limits.h>` included in C++ files

Location: src/devices/runtime_pm.h : 28; src/devices/network.h : 29
Description: Both headers include `<limits.h>` (the C header) rather than the C++ equivalent `<climits>`. In C++ code, C++ standard headers should be preferred.
Suggested fix: Replace `#include <limits.h>` with `#include <climits>`.

### Nit item #3 : C-style `(void)` parameter list in C++ virtual method declarations

Location: src/devices/runtime_pm.h : 46, 47, 49, 51–59; src/devices/network.h : 69–81; src/devices/i915-gpu.h : 40–55
Description: Method declarations use the C idiom `(void)` for parameterless functions (e.g., `virtual void start_measurement(void)`). In C++ the canonical form is empty parentheses `()`. This is a pervasive style inconsistency across all three headers.
Suggested fix: Replace `(void)` with `()` in all C++ method declarations.

### Nit item #4 : Duplicate `#include <unistd.h>` in network.cpp and i915-gpu.cpp

Location: src/devices/network.cpp : 35 and 56; src/devices/i915-gpu.cpp : 31 and 45
Description: `<unistd.h>` is included twice in each of these two translation units. The second inclusion (inside the `extern "C" { }` block in i915-gpu.cpp, and in the second include group in network.cpp) is redundant due to header guards.
Suggested fix: Remove the duplicate `#include <unistd.h>` from each file.

### Nit item #5 : Use `.empty()` instead of `.size()` for emptiness test

Location: src/devices/i915-gpu.h : 48
Description: `if (child_devices.size())` tests whether the vector is non-empty by checking its size. The idiomatic C++ expression is `if (!child_devices.empty())`, which is potentially O(1) and more expressive.
Suggested fix: `if (!child_devices.empty())`

### Nit item #6 : Index-based loop should be a range-based for loop

Location: src/devices/i915-gpu.cpp : 105
Description: `for (unsigned int i = 0; i < child_devices.size(); ++i)` is an index-based loop over a vector. In C++11 and later (this project uses C++20), a range-based for loop is cleaner and avoids the repeated index dereference.
Suggested fix:
```cpp
for (auto *child : child_devices) {
    child_power = child->power_usage(result, bundle);
    if ((power - child_power) > 0.0)
        power -= child_power;
}
```

### Nit item #7 : Redundant `class` keyword in `new` expressions

Location: src/devices/i915-gpu.cpp : 83, 86
Description: `new class i915gpu()` and `new class gpu_rapl_device(gpu)` use the redundant `class` keyword before the type name in `new` expressions. In C++ the `class` keyword is not needed here.
Suggested fix: `gpu = new i915gpu();` and `rapl_dev = new gpu_rapl_device(gpu);`

## Review of src/devices/thinkpad-light.cpp, src/devices/thinkpad-light.h — batch 26

### Nit item #1 : Duplicate `#include <unistd.h>` in thinkpad-light.cpp

Location: src/devices/thinkpad-light.cpp : 32, 44
Description: `<unistd.h>` is included twice; the first occurrence is at line 32 among the other system headers and the second at line 44 at the bottom of the include block.
Suggested fix: Remove the duplicate `#include <unistd.h>` at line 44.

### Nit item #2 : C standard headers appended after project headers — wrong include order

Location: src/devices/thinkpad-light.cpp : 43–44
Description: `#include <string.h>` and `#include <unistd.h>` appear at lines 43–44, after all project-specific headers. Per style.md §4.4, standard-library headers should come first, followed by project headers. These two includes should be moved up with the other system headers (lines 28–33).
Suggested fix: Move `#include <string.h>` to the C-header block at the top of the file.

### Nit item #3 : Stale comment "read the rpms of the light" in `start_measurement()`

Location: src/devices/thinkpad-light.cpp : 57
Description: The comment `/* read the rpms of the light */` is a copy-paste artefact from the fan device. The ThinkPad light has no RPMs; the code reads the LED brightness value.
Suggested fix: Replace with `/* read brightness of the ThinkPad light */` or remove the comment entirely.

### Nit item #4 : Redundant `class` keyword when declaring and allocating the device pointer

Location: src/devices/thinkpad-light.cpp : 77, 86
Description: `class thinkpad_light *light;` and `light = new class thinkpad_light();` use the unnecessary `class` keyword before the type name. This is a C-ism that is not required in C++ and is inconsistent with how allocations are done elsewhere in the codebase.
Suggested fix: `thinkpad_light *light;` and `light = new thinkpad_light();`.
