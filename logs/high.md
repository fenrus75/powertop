# PowerTOP High Severity Review Findings

## Review of calibrate.cpp, calibrate.h, lib.cpp, devlist.cpp, main.cpp — batch 01

### High item #1 : Missing thread joins in calibration functions

Location: src/calibrate/calibrate.cpp : 240–376

Description:
`cpu_calibration()` (line 248) calls `pthread_create` in a loop but re-uses the single `thr` variable, discarding all but the last thread handle. None of the calibration functions — `cpu_calibration`, `wakeup_calibration`, `disk_calibration` — call `pthread_join` before returning. The pattern used is `stop_measurement = 1; sleep(1);` and then returning. The calibration threads may still be running when the next calibration phase begins, or when `calibrate()` returns and the program proceeds. This is a race condition and resource leak.

Severity rationale: Multiple threads reading and writing shared state (`stop_measurement`, I/O, power measurement data) without synchronization can corrupt results or cause crashes.

Suggested fix: Store all created thread handles and call `pthread_join` on each after setting `stop_measurement = 1`. For the CPU calibration loop, store handles in a local `std::vector<pthread_t>`.

---

### High item #2 : `system()` return value check is inverted in calibration functions

Location: src/calibrate/calibrate.cpp : 342–356

Description:
In both `backlight_calibration` and `idle_calibration`, the condition used is:
```cpp
if (!system("DISPLAY=:0 /usr/bin/xset dpms force off"))
    printf("System is not available\n");
```
`system()` returns 0 on success. `!0` evaluates to `true`, so the message is printed when the command **succeeds**, not when it fails. The error message is thus never visible on a system where xset works, and no error is reported when it fails. Additionally, the message `"System is not available\n"` is not wrapped in `_()` and is therefore not translatable.

Severity rationale: Inverted error checking means calibration silently proceeds even when the display power command failed. The user sees a confusing message on success instead.

Suggested fix:
```cpp
if (system("DISPLAY=:0 /usr/bin/xset dpms force off") != 0)
    printf(_("Failed to set display power state\n"));
```

---

### High item #3 : `clean_open_devices()` leaves dangling pointers in vectors

Location: src/devlist.cpp : 76–91

Description:
`clean_open_devices()` deletes all allocated `devuser` and `devpower` objects but never calls `.clear()` on the `one`, `two`, or `devpower` vectors. After the function returns, all three vectors contain dangling pointers. Any subsequent call to `collect_open_devices()` iterates `target` to delete its contents before refilling (lines 107–110), but if `clean_open_devices()` was called first, those deletes operate on already-freed memory — use-after-free.

Severity rationale: Double-free / use-after-free is undefined behaviour and can cause crashes or memory corruption.

Suggested fix: Add `.clear()` calls at the end of each delete loop in `clean_open_devices()`:
```cpp
one.clear();
two.clear();
devpower.clear();
```

---

### High item #4 : `fmt_prefix` — no bounds check before indexing `prefixes[]`

Location: src/lib.cpp : 351–413

Description:
```cpp
static const char prefixes[] = "yzafpnum kMGTPEZY";  // indices 0..16
...
pfx = prefixes[npfx + 8];
```
`npfx` is derived as `((omag + 27) / 3) - 9`. For `omag = -27` → `npfx = -9` → index `-1` (before the array). For `omag >= 27` → index `>= 17` (past the null terminator). The SI prefix table only covers yocto (10⁻²⁴) to yotta (10²⁴), i.e., `omag` must stay in `[-24, +24]`. Any value outside that range — which can occur with very small denormalized or very large floating-point inputs — reads out of bounds.

Severity rationale: Out-of-bounds read from a static array is undefined behaviour; on certain architectures this could return a garbage prefix character or trigger a fault.

Suggested fix: Clamp `npfx` before use:
```cpp
npfx = std::clamp(npfx, -8, 8);
```

---

### High item #5 : `freopen` return-value check is inverted for `-q` (quiet) mode

Location: src/main.cpp : 512–514

Description:
```cpp
case 'q':
    if (freopen("/dev/null", "a", stderr))
        fprintf(stderr, _("Quiet mode failed!\n"));
    break;
```
`freopen` returns the stream pointer on **success** and `NULL` on failure. The condition is true when `freopen` succeeds (quiet mode worked), so the "Quiet mode failed!" message is printed to `/dev/null` (invisible). When `freopen` fails, nothing is printed and `stderr` is left in an indeterminate state (undefined behaviour per C standard). The `!` is missing.

Severity rationale: A `freopen` failure leaves `stderr` in an undefined state; any subsequent `fprintf(stderr, …)` is undefined behaviour. The diagnostic message is also never actually shown.

Suggested fix:
```cpp
case 'q':
    if (!freopen("/dev/null", "a", stderr))
        fprintf(stderr, _("Quiet mode failed!\n"));
    break;
```

## Review of src/test_framework.h, src/test_framework.cpp, src/display.cpp, src/display.h, src/lib.h — batch 02

### High item #1 : `using namespace std;` in display.h header

Location: src/display.h : 33
Description: `using namespace std;` appears in a header file. Because display.h is included by many translation units, this silently injects the entire `std` namespace into every includer's scope, leading to name collisions, ambiguous lookups, and hard-to-diagnose compilation failures. Style rule 4.2 and general-c++.md both explicitly forbid this. The impact in a header is far worse than in a .cpp file.
Severity rationale: Header-file namespace pollution affects the entire build; violates an explicit project rule.
Suggested fix: Remove the `using namespace std;` line and qualify all identifiers with `std::` (e.g., `std::map`, `std::string`).

---

### High item #2 : `using namespace std;` in lib.h header

Location: src/lib.h : 78
Description: Same issue as display.h but worse: lib.h is the project's most widely-included header (pulled in by virtually every translation unit). The `using namespace std;` inside the `#ifdef __cplusplus` block silently exposes the full `std` namespace everywhere. The function declarations on lines 80–84 already benefit from this (`string` instead of `std::string`), making them incorrect as well once the directive is removed.
Severity rationale: Affects the entire codebase; any future std symbol added by a compiler upgrade can silently shadow application-level names.
Suggested fix: Remove `using namespace std;`. Replace bare `string` with `std::string` on the affected `extern` declarations (lines 80–84). Qualified uses like `std::string` on lines 69–93 are already correct.

---

### High item #3 : Missing bounds check in `show_tab()` before indexing `tab_names`

Location: src/display.cpp : 125
Description: `show_tab(unsigned int tab)` immediately indexes `tab_names[tab]` at line 125 without verifying that `tab < tab_names.size()`. An out-of-bounds access invokes undefined behaviour; on a typical STL implementation it will read garbage memory or crash. The function is called directly from external code with a caller-supplied index, making the missing check a real risk.
Severity rationale: Out-of-bounds vector access is undefined behaviour and can crash the process.
Suggested fix:
```cpp
void show_tab(unsigned int tab)
{
if (!display || tab >= tab_names.size())
return;
```

---

### High item #4 : Tab position calculated from internal name length instead of translated name length

Location: src/display.cpp : 143
Description: Inside the loop in `show_tab()`, the rendered string is `tab_translations[tab_names[i]]` (the translated name), but the horizontal advance `tab_pos` is incremented by `tab_names[i].length()` — the length of the *internal* (English) key, not the displayed translation. When a translation is longer than the key, tabs will visually overlap or be misaligned; when shorter, there will be extra gaps. This is a functional display bug.
Severity rationale: Produces incorrect on-screen layout for any locale where translation lengths differ from internal names.
Suggested fix:
```cpp
tab_pos += 3 + tab_translations[tab_names[i]].length();
```

## Review of src/tuning/tunable.h, src/tuning/runtime.h, src/tuning/runtime.cpp — batch 03

### High item #1 : `using namespace std;` in header file tunable.h

Location: src/tuning/tunable.h : 34
Description: `using namespace std;` appears at file scope in a header file. Every translation unit that includes `tunable.h` (directly or transitively) will have the entire `std` namespace injected into its global namespace, causing potential name collisions and violating the project's coding style.
Severity rationale: Header-level namespace pollution is a HIGH issue per style.md §4.2 and general-c++.md — it affects all includers and cannot be undone by the including translation unit.
Suggested fix: Remove the `using namespace std;` line. Replace all unqualified `string` usages in the header with `std::string` (already done for most; the `string` parameters in constructor declarations on line 55 rely on this directive).

### High item #2 : `using namespace std;` in header file runtime.h

Location: src/tuning/runtime.h : 32
Description: Same violation as in tunable.h — `using namespace std;` at file scope in a header file pollutes every includer's namespace.
Severity rationale: Same as item #1. Both `tunable.h` and `runtime.h` have this issue; `runtime.h` also includes `tunable.h`, compounding the pollution.
Suggested fix: Remove the `using namespace std;` line. Qualify all bare `string` references with `std::string`.

### High item #3 : Logic bug — `"on"` returns `TUNE_GOOD` in `good_bad()`

Location: src/tuning/runtime.cpp : 102–103
Description: In `runtime_tunable::good_bad()`, the control value `"on"` (which means the device is forced always-on, disabling runtime power management) incorrectly returns `TUNE_GOOD`. Only `"auto"` (kernel may auto-suspend the device) should be `TUNE_GOOD`; `"on"` should return `TUNE_BAD`. As a result, `toggle()` will always write `"on"` (since `good_bad()` returns `TUNE_GOOD` for both states) and will never write `"auto"`, making it impossible to enable runtime PM via PowerTOP's toggle. The generated `toggle_script()` also always returns the "turn on" script for the same reason.
Severity rationale: This is a functional correctness bug. The core purpose of this class — enabling runtime power management — is broken. Devices stuck in `"on"` mode cannot be toggled to `"auto"` through PowerTOP.
Suggested fix: Change line 102–103 to return `TUNE_BAD` for `"on"`:
```cpp
if (content == "on")
    return TUNE_BAD;
```

## Review of ethernet.cpp, ethernet.h, wifi.cpp, wifi.h, tuningusb.h — batch 04

### High item #1 : `using namespace std;` in header file ethernet.h

Location: src/tuning/ethernet.h : 32
Description: `using namespace std;` is present at file scope in a header. This pollutes the namespace of every translation unit that includes this header, which is explicitly forbidden by the style guide and general-c++ rules. Any file including ethernet.h will have the entire `std` namespace injected.
Severity rationale: The style guide explicitly states "Avoid `using namespace std;`" and general-c++.md marks it as a deprecated construct. Headers are the worst place for this because the pollution is invisible to includers.
Suggested fix: Remove the `using namespace std;` line. The class already correctly uses `std::string` for its members, so no other changes are needed.

### High item #2 : `using namespace std;` in header file wifi.h

Location: src/tuning/wifi.h : 34
Description: Same issue as ethernet.h — `using namespace std;` at file scope in a header contaminates all includers. The constructor parameter already uses `std::string`, and the member uses `std::string`, so the `using` declaration is entirely unnecessary.
Severity rationale: Same as High item #1; prohibited by both style.md and general-c++.md.
Suggested fix: Remove the `using namespace std;` line from wifi.h.

### High item #3 : `using namespace std;` in header file tuningusb.h

Location: src/tuning/tuningusb.h : 33
Description: Same issue — `using namespace std;` at file scope in a public header. All three affected headers (ethernet.h, wifi.h, tuningusb.h) share this violation.
Severity rationale: Same as High items #1 and #2.
Suggested fix: Remove the `using namespace std;` line from tuningusb.h.

### High item #4 : Unchecked `ioctl` return value for `SIOCETHTOOL` in `good_bad()`

Location: src/tuning/ethernet.cpp : 87
Description: The `ioctl(sock, SIOCETHTOOL, &ifr)` call to fetch WoL info (`ETHTOOL_GWOL`) has its return value silently discarded. If the ioctl fails (e.g., the interface does not support ethtool, or an error occurs), `wol.wolopts` will remain zero (from the preceding memset), causing the function to silently return `TUNE_GOOD` — a false "good" result. The interface may actually have WoL enabled and the diagnostic will miss it.
Severity rationale: Incorrect diagnostic result on ioctl failure; the code operates on stale/initialised-to-zero data rather than real device state. This is a logic correctness issue.
Suggested fix:
```cpp
ret = ioctl(sock, SIOCETHTOOL, &ifr);
if (ret < 0) {
    close(sock);
    return TUNE_UNKNOWN;
}
```

### High item #5 : `new` without `std::nothrow` and no null check in `add_wifi_tunables()`

Location: src/tuning/wifi.cpp : 91
Description: `wifi = new class wifi_tunable(entry->d_name);` uses plain `new`, which throws `std::bad_alloc` on allocation failure. Unlike the analogous `ethtunable_callback()` in ethernet.cpp which correctly uses `new(std::nothrow)` followed by a null check, `add_wifi_tunables()` neither catches the exception nor uses the nothrow form. An allocation failure will propagate an unhandled exception, potentially crashing powertop.
Severity rationale: Inconsistency with the established pattern and risk of unhandled exception causing a crash.
Suggested fix:
```cpp
wifi = new(std::nothrow) wifi_tunable(entry->d_name);
if (wifi)
    all_tunables.push_back(wifi);
```

### High item #6 : Wi-Fi interface detection misses modern predictable interface names

Location: src/tuning/wifi.cpp : 90
Description: `strstr(entry->d_name, "wlan")` only matches interfaces whose name contains the substring "wlan" (e.g., `wlan0`). On systems using systemd's predictable network interface naming (the default on virtually all modern Linux distributions), wireless interfaces are named `wlp2s0`, `wlx001122334455`, etc. — none of which contain "wlan". Power saving will silently not be applied to any wireless interface on a modern system.
Severity rationale: Functional regression on modern Linux — power-saving tunable is never created for the majority of real-world Wi-Fi interfaces.
Suggested fix: Check the interface type via `/sys/class/net/<iface>/phy80211` or match using the `wl` prefix that all wireless interfaces share:
```cpp
if (strncmp(entry->d_name, "wl", 2) == 0) {
```

## Review of tuningusb.cpp, tuning.cpp, tuning.h, bluetooth.cpp, bluetooth.h — batch 05

### High item #1 : `using namespace std;` in a header file

Location: src/tuning/bluetooth.h : 32
Description: `using namespace std;` appears at file scope in a public header. This injects the entire `std` namespace into every translation unit that includes bluetooth.h, which can silently cause name clashes or ambiguities that are hard to diagnose. The project style guide explicitly forbids this.
Severity rationale: Header-level namespace pollution affects every consumer of the header. The style guide explicitly prohibits `using namespace std;`.
Suggested fix: Remove the `using namespace std;` line. The only std type used in the class declaration is inherited indirectly via `tunable.h`; bluetooth.h itself does not need the directive.

### High item #2 : Deprecated `hcitool` used to detect active Bluetooth connections

Location: src/tuning/bluetooth.cpp : 144
Description: `popen("/usr/bin/hcitool con 2> /dev/null", "r")` invokes the `hcitool` command, which was deprecated in BlueZ 5.x (circa 2014) and removed from many modern distributions. On systems where `/usr/bin/hcitool` is absent or a stub, popen still succeeds (the shell starts) but produces no useful output, meaning the second fgets call returns NULL immediately. The code then concludes there are no connections and returns `TUNE_GOOD` (BT interface is idle), potentially toggling BT off when it is in active use.
Severity rationale: Silent functional regression on all modern Linux systems — Bluetooth may be disabled while connections are active. The hardcoded path also fails silently if moved.
Suggested fix: Replace with a D-Bus query via the BlueZ 5 API (e.g., `org.bluez.Device1.Connected`), or use `/sys/class/bluetooth/hci0/` sysfs attributes, or at minimum check for `bluetoothctl` before falling back to `hcitool`.

### High item #3 : Redefinition of implementation-reserved `__`-prefixed identifiers

Location: src/tuning/bluetooth.cpp : 60–62
Description: The file defines `#define __u16 uint16_t`, `#define __u8 uint8_t`, and `#define __u32 uint32_t`. All identifiers beginning with `__` (two underscores) are reserved for the C/C++ implementation. Redefining them is undefined behavior and can silently conflict with identically named macros or typedefs in `<stdint.h>`, kernel uapi headers, or glibc, leading to type mismatches or build failures depending on include order.
Severity rationale: Undefined behavior; platform-specific headers already define these names; conflicts will manifest differently per toolchain version.
Suggested fix: Replace the three macros with type aliases using non-reserved names, or simply use `uint8_t`, `uint16_t`, `uint32_t` directly in the struct definitions and remove the `#define` lines.

## Review of src/tuning/tuningsysfs.cpp, src/tuning/tuningsysfs.h, src/tuning/nl80211.h, src/tuning/iw.h, src/tuning/iw.c — batch 06

### High item #1 : Resource leak when NLA_PUT_U32 fails in __handle_cmd

Location: src/tuning/iw.c : 234 / 261
Description: The `NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx)` macro expands to a `goto nla_put_failure` on failure. The `nla_put_failure:` label is placed at line 261, *after* the cleanup labels `out:` (which calls `nl_cb_put(cb)`) and `out_free_msg:` (which calls `nlmsg_free(msg)`). As a result, when `NLA_PUT_U32` fails, execution jumps directly to `nla_put_failure:`, bypassing both cleanup steps. Both `cb` (allocated at line 219) and `msg` (allocated at line 213) are leaked on every such failure path.
Severity rationale: Confirmed resource leak bug on an error path; in a power-management tool that may call this for every wireless interface on every cycle, repeated nl80211 allocation failures could exhaust file descriptors or memory.
Suggested fix: Move the `nla_put_failure:` label *before* `out:` so it falls through into the normal cleanup chain:
```c
 nla_put_failure:
    fprintf(stderr, "building message failed\n");
    err = -ENOBUFS;
 out:
    nl_cb_put(cb);
 out_free_msg:
    nlmsg_free(msg);
    return err;
```

### High item #2 : `using namespace std` in a header file

Location: src/tuning/tuningsysfs.h : 33
Description: `using namespace std;` appears at file scope in the header. This injects the entire `std` namespace into every translation unit that includes `tuningsysfs.h`, including transitively included files. This is explicitly prohibited by both `style.md` ("Avoid `using namespace std;`") and `general-c++.md` ("`using namespace std;` is a deprecated construct"). The effect is especially harmful in a header because it cannot be undone by including files.
Severity rationale: Namespace pollution in a header affects all consumers; can silently cause name collisions that are hard to diagnose.
Suggested fix: Remove line 33 (`using namespace std;`). All uses of `string` in the header already use the fully qualified `std::string`, so no other changes are required.

### High item #3 : Unchecked return value of nla_parse() in print_power_save_handler

Location: src/tuning/iw.c : 150
Description: `nla_parse(attrs, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL)` can return a negative error code if the attribute buffer is malformed. The return value is discarded. If parsing fails, the `attrs` array may be partially or incorrectly filled; accessing `attrs[NL80211_ATTR_PS_STATE]` on the next line then reads potentially uninitialised pointer data.  The practical consequence is that `enable_power_save` silently remains at its previous value rather than being updated to reflect the actual state returned by the kernel.
Severity rationale: Unchecked return value of a function that can fail; leads to silent, incorrect power-save state reporting.
Suggested fix:
```c
if (nla_parse(attrs, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
              genlmsg_attrlen(gnlh, 0), NULL) < 0)
    return NL_SKIP;
```

## Review of tuningi2c.h, tuningi2c.cpp, powerconsumer.h, powerconsumer.cpp, processdevice.h — batch 07

### High item #1 : Division by zero risk on `measurement_time`

Location: src/process/powerconsumer.cpp : 59, 83, 92 and src/process/powerconsumer.h : 68
Description: `measurement_time` is used as a divisor in `Witts()` (line 59), `usage()` (lines 83, 92), and the inline `events()` method (powerconsumer.h:68) without any guard against it being zero. If `measurement_time == 0` (e.g. at startup before the first measurement interval completes, or due to a timing anomaly), these divisions produce `+Inf`, `-Inf`, or `NaN`. These silent bad values then propagate into power estimates and event-rate displays, corrupting every metric shown to the user without any indication that something is wrong.
Severity rationale: Corrupts core power/event metrics silently; NaN values can also trigger undefined behaviour when compared or formatted.
Suggested fix: Guard every division with a check, e.g.:
```cpp
if (measurement_time <= 0.0)
    return 0.0;
cost = cost / measurement_time;
```
Apply the same pattern in `usage()`, `usage_units()`, and `events()`.

## Review of processdevice.cpp, process.h, process.cpp, work.cpp, work.h — batch 08

### High item #1 : Unsigned wraparound before overflow guard corrupts accumulated_runtime

Location: src/process/process.cpp : 63-71
Description: In `process::deschedule_thread()`, `delta = time - running_since` (line 63) is computed *before* the guard `if (time < running_since)` (line 65). Because both `time` and `running_since` are `uint64_t`, when `time < running_since` the subtraction wraps around to a huge positive value. The `printf` on line 66–67 fires after `delta` has already been incorrectly set, and the corrupted `delta` is subsequently added to `accumulated_runtime` on line 71. This silently inflates per-process CPU accounting.
Severity rationale: Produces incorrect measurements that propagate throughout the tool's power estimates; the bug is triggered by any measurement period where a thread's last scheduled-in time straddles a counter reset or wrap.
Suggested fix: Move the overflow check before the subtraction:
```cpp
if (time < running_since) {
    fprintf(stderr, "time %llu < running_since %llu\n",
            (unsigned long long)time, (unsigned long long)running_since);
    running = 0;
    return 0;
}
delta = time - running_since;
```

### High item #2 : Logic error — kernel-process detection is dead code

Location: src/process/process.cpp : 119-127
Description: The outer `if (!cmdline.empty())` (line 119) ensures `cmdline.size() >= 1`, making the inner `if (cmdline.size() < 1)` (line 120) permanently false. The branch that sets `is_kernel = 1` and formats the bracketed description is therefore unreachable. Kernel threads (which have an empty `/proc/<pid>/cmdline`) are caught by the outer `if` being false, meaning neither `is_kernel` nor the bracketed `desc` is ever set from this code path.
Severity rationale: Kernel processes are silently misidentified as user processes; `is_kernel` stays 0 for all processes, breaking any downstream logic that relies on that flag.
Suggested fix: Invert the condition:
```cpp
if (cmdline.empty()) {
    is_kernel = 1;
    desc = std::format("[PID {}] [{}]", pid, comm);
} else {
    cmdline_to_string(cmdline);
    desc = std::format("[PID {}] {}", pid, cmdline);
}
```

## Review of interrupt.h, interrupt.cpp, timer.cpp, timer.h, do_process.cpp — batch 09

### High item #1 : using namespace std in timer.cpp

Location: src/process/timer.cpp : 39
Description: `using namespace std;` is present at file scope in timer.cpp. The project style guide and general-c++ rules both explicitly prohibit this construct. All standard library identifiers must be qualified with the `std::` prefix.
Severity rationale: Explicit prohibition in both style.md and general-c++.md; pollutes the global namespace.
Suggested fix: Remove the `using namespace std;` line and add `std::` qualifiers wherever needed (e.g., `std::map`, `std::pair`, `std::string_view`, `std::string`, `std::for_each`, `std::getline`, `std::istringstream`).

### High item #2 : cpu_idle event: tep_get_field_val return value ignored, val potentially uninitialised

Location: src/process/do_process.cpp : 584
Description: The `cpu_idle` event handler calls `tep_get_field_val(NULL, event, "state", &rec, &val, 0)` without checking the return value. Every other call to `tep_get_field_val` in this function guards on `ret < 0` and returns early. If this call fails, `val` retains an indeterminate value and the subsequent comparison `val == (unsigned int)-1` uses garbage, potentially triggering `set_wakeup_pending` or `consume_blame` incorrectly on every idle event.
Severity rationale: Silently corrupts the wakeup-attribution state for every cpu_idle event when the field read fails; directly affects the accuracy of power-consumer accounting.
Suggested fix:
```cpp
ret = tep_get_field_val(NULL, event, "state", &rec, &val, 0);
if (ret < 0)
    return;
if (val == (unsigned int)-1)
    ...
```

### High item #3 : Division by zero — measurement_time not guarded

Location: src/process/do_process.cpp : 723
Description: `measurement_time` (a global `double`) is zero-initialised and is only updated inside `handle_trace_point` when a trace event with `time > last_stamp` arrives. If no trace events are captured during a measurement cycle (e.g., on the very first run or on a system with all tracepoints disabled), `measurement_time` remains 0.0. All of `total_wakeups()`, `total_gpu_ops()`, `total_disk_hits()`, `total_hard_disk_hits()`, `total_xwakes()` (lines 716–784) and several inline divisions in `report_process_update_display()` (lines 963–973) divide by `measurement_time`, producing infinity or NaN values that propagate to display output and report data.
Severity rationale: Directly corrupts displayed data on any measurement run that produces no events; NaN/Inf values can cause undefined behaviour in subsequent arithmetic.
Suggested fix: Guard each division site, e.g.: `if (measurement_time > 0.0) total = total / measurement_time;`

### High item #4 : static prev_time persists across measurement cycles

Location: src/process/do_process.cpp : 631
Description: `prev_time` is declared `static uint64_t` inside `handle_trace_point`, meaning it survives between complete measurement cycles. The `hard_disk_hits` logic fires when `(time - prev_time) > 1000000000`, but `prev_time` carries over from the previous measurement window. The first disk event in a new measurement cycle will almost always satisfy the condition because the gap since the last event in the prior cycle is typically > 1 second, inflating `hard_disk_hits` counts spuriously.
Severity rationale: Directly corrupts `hard_disk_hits` accounting on every measurement cycle after the first; this is a primary metric shown to users.
Suggested fix: Reset `prev_time = 0` at the start of each measurement cycle (e.g., in `start_process_measurement()` or `process_process_data()`), or store it in a per-measurement-cycle structure.

### High item #5 : timer_is_deferred reads /proc/timer_stats which was removed in Linux 4.11

Location: src/process/timer.cpp : 44
Description: `timer_is_deferred()` attempts to read `/proc/timer_stats` to determine whether a timer uses the deferrable flag. This procfs file was removed in Linux kernel 4.11 (2017). On all modern kernels the file does not exist; `read_file_content` returns an empty string and the function always returns `false`. The `deferred` field is set in the constructor but is permanently `false`, and `is_deferred()` has no useful effect, yet it silently skips adding deferred timers in `timer_expire_entry` handling.
Severity rationale: Functionality is permanently inoperative on any kernel >= 4.11 (essentially all current distributions); the silent no-op may mask accounting bugs.
Suggested fix: Remove `timer_is_deferred()` and the `deferred` field, or replace with a mechanism that works on current kernels (or simply always return false explicitly and document the limitation).

### High item #6 : Copy-paste error in error message — wrong event name

Location: src/process/do_process.cpp : 457
Description: Inside the `timer_expire_entry` handler, the error message reads `"softirq_entry event returned no timer ?"`. This is a copy-paste from the nearby `softirq_entry` block. The message is misleading for any developer debugging a missing timer field in a `timer_expire_entry` event.
Severity rationale: Wrong diagnostic information emitted to stderr; impedes debugging of a real error.
Suggested fix: Change to `"timer_expire_entry event returned no timer?\n"`.

### High item #7 : all_timers_to_all_power adds all timers regardless of accumulated_runtime

Location: src/process/timer.cpp : 117
Description: `all_timers_to_all_power()` unconditionally pushes every timer from `all_timers` into `all_power`. By contrast, `all_interrupts_to_all_power()` (interrupt.cpp:117) filters on `accumulated_runtime > 0`. Zero-runtime timers will appear in the overview display and all accounting functions, distorting wakeup counts, sort order, and report tables.
Severity rationale: Directly inflates the `all_power` list with zero-contribution entries that show in the user-visible overview, degrading display accuracy.
Suggested fix:
```cpp
void all_timers_to_all_power(void)
{
    for (auto &[addr, t] : all_timers)
        if (t->accumulated_runtime)
            all_power.push_back(t);
}
```

## Review of src/perf/perf.h, src/perf/perf_event.h, src/perf/perf_bundle.cpp, src/perf/perf_bundle.h, src/perf/perf.cpp — batch 10

### High item #1 : `perf_fd` left open and `pc` left NULL on mmap failure in `create_perf_event`

Location: src/perf/perf.cpp : 119
Description: When `mmap()` fails, the function prints an error and returns, but `perf_fd` (already assigned a valid fd at line 95) is never closed, and `pc` / `data_mmap` are never set. This causes two problems: (1) a file descriptor leak on every mmap failure, and (2) a subsequent null pointer dereference in `process()` as described in Critical item #2.
Severity rationale: File descriptor leaks accumulate over the lifetime of the program and can exhaust OS limits. The combination with the null pointer dereference makes this a high-severity resource management bug.
Suggested fix:
```cpp
if (perf_mmap == MAP_FAILED) {
    fprintf(stderr, _("failed to mmap perf buffer: %d (%s)\n"), errno, strerror(errno));
    close(perf_fd);
    perf_fd = -1;
    return;
}
```

### High item #2 : `perf_event` destructor does not call `clear()` in the else branch — resource leak

Location: src/perf/perf.cpp : 167
Description: The destructor only calls `clear()` when the last reference to `tep` is being released (i.e., `tep_get_ref() == 1`). In the `else` branch (more references exist), `tep_unref()` is called but `clear()` is never invoked. As a result, the `perf_fd` file descriptor and the `perf_mmap` memory mapping are leaked for every `perf_event` object that is destroyed while other `perf_event` objects still exist.
Severity rationale: In normal operation, many `perf_event` objects share the `tep` handle, so the else branch is the common path. Every destroyed event except the very last will leak an fd and an mmap region.
Suggested fix:
```cpp
perf_event::~perf_event(void)
{
    clear();
    if (tep_get_ref(perf_event::tep) == 1) {
        tep_free(perf_event::tep);
        perf_event::tep = NULL;
    } else {
        tep_unref(perf_event::tep);
    }
}
```

### High item #3 : Missing memory barrier after reading `data_head` in `perf_event::process()`

Location: src/perf/perf.cpp : 231
Description: The `perf_event_mmap_page` documentation (present in `perf_event.h` lines 299-308) explicitly states: "User-space reading the @data_head value should issue an rmb(), on SMP capable platforms, after reading this value." The `process()` loop reads `pc->data_head` and immediately accesses ring buffer data without any read memory barrier (`rmb()` / `__sync_synchronize()` / `std::atomic_thread_fence`). On SMP systems the kernel may still be writing the data, causing the process to read stale or incomplete event data.
Severity rationale: On any SMP machine (the typical PowerTOP target), this can silently corrupt perf event processing, leading to incorrect power analysis results.
Suggested fix: Insert `__sync_synchronize()` (or `asm volatile("" ::: "memory")`) immediately after reading `pc->data_head` and before accessing `data_mmap`.

## Review of persistent.cpp, parameters.cpp, parameters.h, learn.cpp, report-formatter.h — batch 11

### High item #1 : `using namespace std` in header file parameters.h

Location: src/parameters/parameters.h : 37
Description: `using namespace std;` appears in parameters.h, which is a shared header included by most of the codebase. This silently injects the entire `std` namespace into every translation unit that includes parameters.h (directly or transitively). This is explicitly forbidden by both style.md ("Avoid `using namespace std;`") and general-c++.md ("`using namespace std;` is a deprecated construct"). In a header file the damage is amplified because the developer of any including .cpp file has no visible indication that `std` has been imported.
Severity rationale: Header-level namespace pollution affects the entire codebase. It can cause silent name collisions and is an explicit rule violation.
Suggested fix: Remove `using namespace std;` from parameters.h and qualify all unqualified `map`, `vector`, `string` etc. references with the `std::` prefix.

### High item #2 : `using namespace std` in header file report-formatter.h

Location: src/report/report-formatter.h : 32
Description: Same violation as in parameters.h — `using namespace std;` in a header file forces the entire `std` namespace into every translation unit that includes this header.
Severity rationale: Same as item #1. Header-level namespace pollution, explicit rule violation.
Suggested fix: Remove `using namespace std;` and prefix all standard-library types with `std::`.

### High item #3 : Acknowledged but unfixed memory leaks in `store_results` and `load_results`

Location: src/parameters/parameters.cpp : 342 ; src/parameters/persistent.cpp : 117
Description: In both `store_results` (parameters.cpp:342) and `load_results` (persistent.cpp:117), when `past_results` has reached `MAX_PARAM` capacity the code overwrites a slot with a new pointer without freeing the old one. Both sites are annotated with `/* memory leak, must free old one first */`. Because this path is taken on every new measurement after the buffer is full, the leak is unbounded over a long PowerTOP session. The correct fix is one additional `delete past_results[overflow_index];` before the assignment.
Severity rationale: Unbounded memory growth during normal long-running operation.
Suggested fix:
```cpp
// Before:
past_results[overflow_index] = clone_results(&all_results);
// After:
delete past_results[overflow_index];
past_results[overflow_index] = clone_results(&all_results);
```
Apply the same pattern in `load_results`.

### High item #4 : `get_parameter_weight` has no bounds check — undefined behaviour

Location: src/parameters/parameters.cpp : 129
Description: `get_parameter_weight(int index, struct parameter_bundle *the_bundle)` directly returns `the_bundle->weights[index]` without checking that `index` is non-negative or that `index < the_bundle->weights.size()`. An out-of-range index causes undefined behaviour (likely a crash with debug iterators, silent data corruption otherwise). The neighbouring `get_parameter_value(unsigned int index, …)` at line 120 does perform this check and emits a "BUG:" message.
Severity rationale: Silent UB / potential crash in production code; no bounds guard exists.
Suggested fix:
```cpp
double get_parameter_weight(int index, struct parameter_bundle *the_bundle)
{
    if (index < 0 || (unsigned int)index >= the_bundle->weights.size()) {
        fprintf(stderr, "BUG: requesting unregistered weight %d\n", index);
        return 1.0;
    }
    return the_bundle->weights[index];
}
```

### High item #5 : `utilization_power_valid(const std::string &u)` accesses `past_results[0]` without empty check

Location: src/parameters/parameters.cpp : 401
Description: The string-parameter overload of `utilization_power_valid` accesses `past_results[0]->utilization[index]` unconditionally. If `past_results` is empty this is an out-of-bounds vector access — undefined behaviour, typically a crash. The integer-parameter overload of the same function at line 420 does correctly guard with `if (past_results.size() == 0) return 0;`.
Severity rationale: Missing guard that the sister overload has, leading to UB / crash if called before any results are stored.
Suggested fix: Add `if (past_results.size() == 0) return 0;` immediately after the `if (index <= 0) return 0;` check on line 399.

## Review of report-formatter-html.h, report.cpp, report.h, report-formatter-csv.h, report-maker.cpp — batch 12

### High item #1 : `using namespace std` in header files pollutes all includers

Location: report.h : 35, report-formatter-html.h : 35, report-formatter-csv.h : 46
Description: All three header files contain `using namespace std;` at file scope. Per the C++ coding guidelines this is a deprecated construct, and placing it in a header is especially harmful: every translation unit that includes these headers (directly or transitively) has the entire `std` namespace injected, making it impossible for downstream code to keep the namespaces separate. The style guide also mandates the explicit `std::` prefix at all times.
Severity rationale: In a header file the pollution is unconditional and affects all includers; it is a correctness hazard (silent name-hiding / ADL surprises) that is impossible for consumers to work around.
Suggested fix: Remove the `using namespace std;` line from all three headers. Replace bare `string` with `std::string` where it appears in those headers (e.g., `report-formatter-html.h:68`, `report-formatter-csv.h:67`).

### High item #2 : Non-translatable user-visible string in report output

Location: report.cpp : 131
Description: `system_data[1]` is built with `std::format("{} ran at {}", PACKAGE_VERSION, get_time_string("%c", now))`. The phrase "ran at" is directly visible in the generated report but is never passed through the gettext `_()` macro, so it will always appear in English regardless of the user's locale. The style guide requires that all user-visible strings be wrapped in `_()` and that user-facing formatted strings use `pt_format` rather than `std::format`.
Severity rationale: Explicit rule: "All user visible strings must be translatable and use the `_()` gettext pattern." This affects all non-English users every time a report is produced.
Suggested fix: Replace with `pt_format(_("{} ran at {}"), PACKAGE_VERSION, get_time_string("%c", now))`.

### High item #3 : `localtime()` return value not checked before use

Location: report.cpp : 105-106
Description: `localtime(&t)` can return `NULL` (e.g., for an out-of-range `time_t` value). The return value is stored in `tm_info` but is passed directly to `strftime` without a null check. Passing a null pointer to `strftime` is undefined behaviour and will cause a crash.
Severity rationale: Undefined behaviour / potential null-pointer dereference in code that runs on every report-generating invocation.
Suggested fix:
```cpp
struct tm *tm_info = localtime(&t);
if (!tm_info)
    return "";
if (strftime(buf, sizeof(buf), fmt.c_str(), tm_info))
    return buf;
return "";
```

## Review of report-maker.h, report-data-html.cpp, report-data-html.h, report-formatter-base.cpp, report-formatter-base.h — batch 13

### High item #1 : `double_to_string` — unsigned wrap on `npos+2` produces single-character output for integer-valued doubles

Location: src/report/report-data-html.cpp : 124
Description: `str.find(".")` returns `std::string::npos` (the maximum `size_t` value) when the formatted double contains no decimal point (e.g., `100.0` formats as `"100"` via `ostringstream`). The expression `str.find(".")+2` then wraps around via unsigned arithmetic to `1`. Consequently `str.substr(0, 1)` is returned — only the first character of the number. For example, a power value of `100.0` W would display as `"1"` in the HTML report. Any integer-valued double triggers this bug.
Severity rationale: Produces visibly wrong numeric data in the generated power report for any integer-valued measurement.
Suggested fix:
```cpp
std::string double_to_string(double dval)
{
std::ostringstream dtmp;
dtmp << dval;
std::string str = dtmp.str();
auto dot = str.find('.');
if (dot != std::string::npos)
str = str.substr(0, dot + 2);
return str;
}
```

### High item #2 : `using namespace std;` in header files

Location: src/report/report-maker.h : 65  and  src/report/report-data-html.h : 7
Description: Both headers contain `using namespace std;` at file scope. Because these are headers, every translation unit that includes them (directly or transitively) has the entire `std` namespace injected without opt-in. This can cause silent name collisions, ambiguous overload sets, and violates the project's own style guide (style.md §4.2) and the general C++ rules.
Severity rationale: Namespace pollution that silently affects all includers, a first-class style violation in both the style guide and general-c++ rules.
Suggested fix: Remove `using namespace std;` from both headers and qualify all standard-library names with `std::`.

## Review of report-formatter-csv.cpp, report-formatter-html.cpp, dram_rapl_device.cpp, dram_rapl_device.h, cpu_linux.cpp — batch 14

### High item #1 : `csv_need_quotes` member variable used uninitialized

Location: src/report/report-formatter-csv.cpp : 67  (also src/report/report-formatter-csv.h : 68)
Description: The `csv_need_quotes` bool member of `report_formatter_csv` is declared in the header but never initialized — not in the constructor, not in the member initializer list. The constructor body contains only `/* Do nothing special */`. When `escape_string()` sets `csv_need_quotes = true` on line 67, it only ever writes to it; any code that reads `csv_need_quotes` (e.g., the declared-but-unimplemented `add_quotes()`) would read garbage. Reading an uninitialized bool is undefined behavior in C++. The companion member `text_start` (size_t) suffers the same defect.
Severity rationale: Uninitialized member variable read is undefined behavior. While `add_quotes()` is currently not implemented, the member is part of the class invariant and any future reader would encounter UB. It also signals incomplete/broken initialization.
Suggested fix: Initialize both members in the constructor member initializer list:
```cpp
report_formatter_csv::report_formatter_csv()
    : csv_need_quotes(false), text_start(0)
{}
```

### High item #2 : Out-of-bounds access in `add_summary_list` for odd-length vectors

Location: src/report/report-formatter-html.cpp : 318-320
Description: `add_summary_list` iterates over a `std::vector<std::string>` in steps of 2 (`i+=2`) and unconditionally accesses both `list[i]` and `list[i+1]` inside the loop. If the caller passes a vector with an odd number of elements, the last iteration accesses `list[i+1]` which is one past the end — undefined behavior and likely a crash or memory corruption. There is no assertion or bounds check guarding this. The same pattern in `report-formatter-csv.cpp` line 129-131 partially guards with `if(i < (list.size() - 1))` but that check itself underflows when `list.size() == 0` (size_t wraps to SIZE_MAX) — though harmless because the loop doesn't execute.
Severity rationale: Out-of-bounds vector access is undefined behavior that can cause crashes or silent memory corruption in normal program operation if any caller provides an odd-length list.
Suggested fix:
```cpp
for (size_t i = 0; i + 1 < list.size(); i += 2) {
    add_exact(std::format("...", list[i], list[i+1]));
}
```

## Review of src/cpu/cpu_core.cpp, src/cpu/cpudevice.h, src/cpu/cpudevice.cpp, src/cpu/abstract_cpu.cpp, src/cpu/cpu_rapl_device.cpp — batch 15

### High item #1 : `using namespace std` in header file cpudevice.h

Location: src/cpu/cpudevice.h : 31
Description: `using namespace std;` appears at file scope in a header file. This is explicitly
forbidden by both style.md (§4.2) and general-c++.md. Placing it in a header silently pollutes
the global namespace for every translation unit that includes this header directly or transitively,
making it far worse than the same statement in a .cpp file.
Severity rationale: Header-scope `using namespace std` is explicitly banned and causes broad,
hard-to-diagnose name collision risks across the entire codebase.
Suggested fix: Remove the `using namespace std;` line. All standard types used in the header
already carry explicit `std::` prefixes (e.g. `std::string`, `std::vector`), so no further
changes are needed to make the header compile correctly.

### High item #2 : `using namespace std` in header file cpu_rapl_device.h

Location: src/cpu/cpu_rapl_device.h : 31
Description: Same violation as cpudevice.h — `using namespace std;` at file scope in a header.
Every file that includes `cpu_rapl_device.h` (directly or via cpudevice.h) inherits this
namespace pollution.
Severity rationale: Same as item #1; header-scope `using namespace std` is explicitly banned.
Suggested fix: Remove the `using namespace std;` line. All identifiers in the header already
use `std::` qualifiers or are fully spelled out.

## Review of src/cpu/cpu_rapl_device.h, src/cpu/cpu_package.cpp, src/cpu/cpu.h, src/cpu/cpu.cpp, src/cpu/intel_cpus.cpp — batch 16

### High item #1 : Division by zero in report_display_cpu_cstates when num_cores == 0

Location: src/cpu/cpu.cpp : 500
Description: `cpu_tbl_size.cols = (2 * (num_cpus / num_cores)) + 1` is computed after
counting only children whose `get_type()` returns exactly `"Core"`. GPU cores
(type `"GPU"`) are present in the same children vector (added by `new_i965_gpu`).
If a package contains only a GPU child or all `_core` pointers happen to be NULL,
`num_cores` stays 0 and the integer division causes a divide-by-zero crash.
Severity rationale: Any system with an integrated GPU (the i965 path) combined
with certain topology configurations will hit this.
Suggested fix: Guard the division: `cpu_tbl_size.cols = num_cores > 0 ? (2 * (num_cpus / num_cores)) + 1 : 1;`

### High item #2 : Division by zero in report_display_cpu_pstates when num_cores == 0

Location: src/cpu/cpu.cpp : 708
Description: Same pattern as High item #1 in `report_display_cpu_pstates`:
`cpu_tbl_size.cols = (num_cpus / num_cores) + 1` when `num_cores == 0` is a
divide-by-zero crash.
Severity rationale: Same reasoning as High item #1; both report paths are
reachable from normal operation.
Suggested fix: Guard as above: `cpu_tbl_size.cols = num_cores > 0 ? (num_cpus / num_cores) + 1 : 1;`

### High item #3 : Null pointer dereference of `_core` after inner loop in report_display_cpu_cstates

Location: src/cpu/cpu.cpp : 620
Description: `_core` is declared as `NULL` at line 437. Inside the `for (core = ...)` loop it is
assigned `_core = _package->children[core]`; when the child is NULL the loop
`continue`s. After the loop `_core` holds the value from the last *executed*
assignment, which may itself be NULL (if the last child slot is a NULL sentinel).
At line 620, `_core->can_collapse()` is then called unconditionally, causing a
null pointer dereference. The same pointer is used at line 621 as well.
Severity rationale: Any package whose last indexed child slot is a gap (NULL)
triggers this; this can happen with non-contiguous core numbering.
Suggested fix: Test before use: `if (_core && !_core->can_collapse()) { ... }`

### High item #4 : `using namespace std;` in header files

Location: src/cpu/cpu.h : 36  and  src/cpu/cpu_rapl_device.h : 31
Description: Both header files contain `using namespace std;` at file scope.
This is explicitly prohibited by the project style guide and the C++ coding
guidelines ("'using namespace std;' is a deprecated construct"). Being in headers,
this silently forces every translation unit that includes these widely-used headers
to import the entire `std` namespace, creating hidden name-collision hazards and
polluting the namespace of all downstream code.
Severity rationale: Affects every file in the project that transitively includes
these headers; a name conflict introduced by a future standard library addition
would be extremely hard to diagnose.
Suggested fix: Remove both `using namespace std;` lines and qualify all bare uses
of `vector`, `string`, `cout` etc. with `std::`.

## Review of src/cpu/rapl/rapl_interface.cpp — batch 17

### High item #1 : readdir loop does not skip "." and ".." entries (first loop)

Location: src/cpu/rapl/rapl_interface.cpp : 98
Description: The `while ((entry = readdir(dir)) != NULL)` loop starting at line 98 iterates over all entries in `/sys/class/powercap/intel-rapl/` without skipping the special entries "." and "..". The code constructs paths such as `base_path + entry->d_name + "/name"`, which for "." yields `.../intel-rapl/./name` (resolves to the parent directory's `name` file) and for ".." yields `.../powercap/name`. While `read_sysfs_string` likely returns an empty string for these nonexistent paths, this is an explicit violation of the rules.md requirement and could potentially read unintended sysfs files if the filesystem layout changes.
Severity rationale: Explicit violation of the rules.md directory-traversal rule. Low-probability but not zero risk of reading unintended paths.
Suggested fix: Add a check at the top of the loop body: `if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;`

### High item #2 : readdir loop does not skip "." and ".." entries (second loop)

Location: src/cpu/rapl/rapl_interface.cpp : 116
Description: The second `while ((entry = readdir(dir)) != NULL)` loop at line 116 iterates over a package subdirectory without skipping "." and "..". It constructs `path = package_path + entry->d_name` and then reads `path + "/name"`. For ".." this would resolve to the parent path `/sys/class/powercap/intel-rapl/<entry>/name`, potentially matching a domain name of "core", "dram", or "uncore" in unexpected locations if the sysfs tree structure were ever to include such files at the parent level.
Severity rationale: Same rule violation as item #1; the second loop compounds the issue.
Suggested fix: Same as item #1 — add a "." / ".." skip guard at the top of the loop body.

## Review of wakeup_usb.h, wakeup_ethernet.h, waketab.cpp, wakeup_ethernet.cpp, wakeup.cpp — batch 18

### High item #1 : stray semicolon at start of wakeup_ethernet.cpp

Location: src/wakeup/wakeup_ethernet.cpp : 1
Description: The file begins with `;/*` — a bare semicolon before the copyright comment. While C++11 permits empty declarations at namespace scope, this is clearly a typo/corruption that has no business being there. It confuses source tools, diff viewers, and static analysers, and is a latent sign of file-level editing damage.
Severity rationale: High — it is an unambiguous file corruption that affects every build. It is only not critical because C++11 empty-declaration rules save it from a compile error.
Suggested fix: Remove the leading `;` so the file starts with `/*`.

### High item #2 : out-of-bounds vector access in cursor_enter when wakeup_all is empty

Location: src/wakeup/waketab.cpp : 46, 105
Description: `win->cursor_max` is assigned `wakeup_all.size() - 1`. When `wakeup_all` is empty, `size_t(0) - 1` wraps to `SIZE_MAX`; the implicit narrowing conversion to `int cursor_max` yields -1. The navigation guards (`cursor_pos < cursor_max`) therefore prevent moving the cursor, but `cursor_pos` is initialised to 0 and `cursor_enter()` is called unconditionally on Enter. Inside `cursor_enter`, `wakeup_all[cursor_pos]` (line 105) accesses `wakeup_all[0]` on an empty vector — undefined behaviour that in practice crashes.
Severity rationale: High — any system with no wakeup-capable devices will crash on Enter in the WakeUp tab. The unsigned-to-signed truncation also silently suppresses any diagnostic.
Suggested fix:
```cpp
win->cursor_max = wakeup_all.empty() ? 0 : (int)wakeup_all.size() - 1;
```
And add a bounds guard in `cursor_enter`:
```cpp
if ((size_t)cursor_pos >= wakeup_all.size())
    return;
```

### High item #3 : `using namespace std` in headers

Location: src/wakeup/wakeup_usb.h : 32  and  src/wakeup/wakeup_ethernet.h : 32
Description: Both headers contain `using namespace std;` at file scope. This silently injects the entire `std` namespace into every translation unit that includes them, potentially causing name collisions or hiding bugs in unrelated code. The coding style guide explicitly forbids this.
Severity rationale: High — namespace pollution from a header affects all includers and is a hard style-guide violation.
Suggested fix: Remove both `using namespace std;` lines. The classes already use fully-qualified `std::string` everywhere they need it.

## Review of src/measurement/acpi.cpp — batch 19

### High item #1 : `/proc/acpi/battery/` path removed from Linux kernel

Location: src/measurement/acpi.cpp : 74
Description: The `acpi_power_meter::measure()` function reads from `/proc/acpi/battery/{name}/state`. This `/proc/acpi/battery/` interface was removed from the Linux kernel in version 2.6.39 (released April 2011). On any kernel shipped in the past decade, `read_file_content()` will always return an empty string, causing `measure()` to return immediately without setting any values. As a result, `power()` always returns 0.0 — the entire `acpi_power_meter` class is silently non-functional on modern systems. The replacement interface is `/sys/class/power_supply/` with the `energy_now`, `power_now`, `voltage_now`, etc. attributes.
Severity rationale: High — the entire measurement class fails silently on all modern kernels; users relying on ACPI battery data receive no power readings.
Suggested fix: Rewrite `measure()` to use `/sys/class/power_supply/{name}/power_now` (or `current_now` × `voltage_now`) from the sysfs power supply class. Remove the `/proc/acpi/battery/` parsing entirely.

## Review of sysfs.h, sysfs.cpp, measurement.h, measurement.cpp, extech.h — batch 20

### High item #1 : `using namespace std;` in a header file

Location: src/measurement/measurement.h : 31
Description: `using namespace std;` appears at global scope in a header file. Every translation unit that includes `measurement.h` (directly or indirectly) inherits this namespace pollution. This is explicitly forbidden by general-c++.md ("using namespace std; is a deprecated construct") and style.md §4.2. It is also the root cause of unqualified `string` and `vector` usage in sysfs.cpp (lines 32, 38) and measurement.cpp (line 58).
Severity rationale: Broad, silent namespace pollution that violates an explicit project rule and can cause unexpected name collisions in any including file.
Suggested fix: Remove `using namespace std;` from measurement.h. Replace `vector<class power_meter *>` on line 61 with `std::vector<power_meter *>`. Update all callers that relied on the implicit `using namespace std;` (sysfs.cpp lines 32/38 and measurement.cpp line 58) to use fully qualified `std::string` and `std::vector`.

### High item #2 : `CLOCK_REALTIME` used for elapsed-time measurement

Location: src/measurement/measurement.cpp : 65, 102
Description: Both `start_power_measurement` (line 65) and `global_sample_power` (line 102) call `clock_gettime(CLOCK_REALTIME, ...)` to measure elapsed wall-clock time. `CLOCK_REALTIME` can jump forward or backward due to NTP corrections, leap-second smearing, or manual adjustments. A backward jump makes `tnow - tlast` negative, accumulating negative joules. A large forward jump inflates the joules figure by hours of phantom energy. `CLOCK_MONOTONIC` is the correct clock for measuring elapsed time intervals.
Severity rationale: Directly corrupts the joules accumulation used for battery life estimates whenever the system clock is adjusted during a measurement period.
Suggested fix: Replace `CLOCK_REALTIME` with `CLOCK_MONOTONIC` at both call sites.

### High item #3 : Data race on `sum` and `samples` in extech_power_meter

Location: src/measurement/extech.h : 36-37
Description: `sum` (double) and `samples` (int) are written by the sampling thread (`sample()` in extech.cpp) and read by the main thread in `end_measurement()` without any mutex, lock, or atomic type. Under the C++20 memory model this is a data race and constitutes undefined behavior. On architectures that do not guarantee atomic double/int reads the final `rate = sum / samples` can observe a torn value.
Severity rationale: Undefined behavior triggered on every use of an Extech power meter device; can produce garbage power readings or crash.
Suggested fix: Use `std::atomic<double> sum` and `std::atomic<int> samples`, or protect access with a `std::mutex`. Since `std::atomic<double>` only became lock-free on common hardware in C++20, a mutex is the safer portable choice.

## Review of src/measurement/extech.cpp, src/devices/backlight.cpp — batch 21

### High item #1 : Thread data races on shared variables in extech sampling thread

Location: src/measurement/extech.cpp : 311-312, 330-333, 341-342
Description: The member variables `sum`, `samples`, and `end_thread` are accessed from two threads concurrently — the sampling thread (via `sample()`) and the main thread (via `start_measurement()` and `end_measurement()`) — with no synchronization whatsoever. `end_thread` is a plain `int` used as a thread-termination flag, which the compiler may cache in a register and never re-read from memory. Reading and writing these variables from multiple threads without atomics or mutexes is undefined behavior under the C++ memory model.
Severity rationale: Data races are undefined behavior in C++20. The race on `end_thread` can cause the sampling thread to loop forever; races on `sum`/`samples` can produce torn reads and corrupt power-usage values reported to users.
Suggested fix: Declare `end_thread` as `std::atomic<int>` (or `std::atomic<bool>`). Protect `sum` and `samples` with a `std::mutex`, or use `std::atomic<double>`/`std::atomic<int>` for them.

### High item #2 : Division by zero in backlight::utilization() and end_measurement() when max_level is 0

Location: src/devices/backlight.cpp : 102, 117
Description: Both `backlight::end_measurement()` (line 102) and `backlight::utilization()` (line 117) compute `100.0 * (...) / max_level`. `max_level` is read from sysfs in `start_measurement()` and is initialized to 0 in the constructor. If the sysfs read fails, or if `start_measurement()` is never called, `max_level` remains 0, causing a division by zero (or, for IEEE 754 double, a ±inf/NaN result) that propagates into power estimates.
Severity rationale: A missing or unreadable sysfs file causes a divide-by-zero, producing meaningless (inf/NaN) power values that corrupt downstream calculations and user-visible output.
Suggested fix: Guard both call sites: `if (max_level <= 0) return 0.0;` before the division.

### High item #3 : Error sentinel values from extech_read() silently accumulated into sum

Location: src/measurement/extech.cpp : 311
Description: `extech_read()` returns `-1.0` on select timeout/error and `-1000.0` on parse failure. In `sample()`, the return value is unconditionally added to `sum` with `sum += extech_read(fd)`. Whenever the device fails to respond or returns a bad packet, a large negative value is folded into the running average, producing a badly wrong `rate` once `end_measurement()` computes `sum / samples`.
Severity rationale: Error readings directly contaminate the power measurement average, leading to incorrect (heavily negative) watt values shown to the user.
Suggested fix: Check the return value before accumulating: `double v = extech_read(fd); if (v >= 0.0) { sum += v; samples++; }` (and remove the separate `samples++` on line 312).

## Review of src/devices/usb.h, src/devices/usb.cpp, src/devices/gpu_rapl_device.h, src/devices/gpu_rapl_device.cpp, src/devices/ahci.h — batch 22

### High item #1 : `using namespace std;` in header file gpu_rapl_device.h

Location: src/devices/gpu_rapl_device.h : 31
Description: The file contains `using namespace std;` at global scope in a header. This is explicitly forbidden by both `style.md` (§4.2) and `general-c++.md`. When placed in a header file, it silently injects the entire `std` namespace into every translation unit that includes this header, causing hard-to-diagnose name collisions and making it impossible for includers to opt out. All other headers in the project use the `std::` prefix convention; this one is an outlier.
Severity rationale: Header-level `using namespace std` pollutes all includers' namespaces and is explicitly listed as a deprecated construct in the coding rules. The impact is project-wide.
Suggested fix: Remove `using namespace std;` and prefix all standard-library types in the header with `std::`. Because the header itself only uses `std::string` and `std::vector` (via the includes), the fix is mechanical: drop line 31 and verify no bare `string` or `vector` names remain in the header.

### High item #2 : `power_valid()` can return values other than 0/1 in ahci.h

Location: src/devices/ahci.h : 63
Description: The method `power_valid()` returns `utilization_power_valid(partial_rindex) + utilization_power_valid(active_rindex)`. Each `utilization_power_valid()` call returns 0 or 1. When both AHCI states are valid, the sum is 2. The base-class `device::power_valid()` contract returns an int treated as a boolean flag; callers that check `power_valid() == 1` or cast to bool (which is fine), but callers checking `!= 0` also work. However the semantics of "how many sub-metrics are valid" vs "is power valid at all" are conflated. If the intent is "power is valid if either partial or active is valid" the expression should be `||` not `+`; if it means "valid only when both are valid" the expression should be `&&`. Neither intention produces the value 2.
Severity rationale: Incorrect boolean arithmetic in a validation predicate that controls whether power data is displayed to the user. Could cause incorrect display behavior depending on caller comparison style.
Suggested fix: Change to: `return utilization_power_valid(partial_rindex) || utilization_power_valid(active_rindex);` (or `&&` if both are required).


## Review of src/devices/ahci.cpp, src/devices/thinkpad-fan.h, src/devices/thinkpad-fan.cpp, src/devices/device.h, src/devices/device.cpp — batch 23

### High item #1 : `using namespace std;` in a header file

Location: src/devices/device.h : 74
Description: `using namespace std;` appears at the end of device.h, after the class definition. Because this is a header file, every translation unit that includes device.h silently inherits this directive. This can cause hard-to-diagnose name collisions with std names (e.g. `string`, `vector`, `sort`) silently resolving to `std::` symbols when the programmer may intend otherwise. The `std::` prefix is already used correctly throughout the class definition above it, making this directive entirely unnecessary.
Severity rationale: Placing `using namespace std;` in a header is explicitly prohibited by style.md §4.2 and general-c++.md ("using namespace std; is a deprecated construct"). Its presence in a header is strictly worse than in a .cpp file because it contaminates all includers transitively.
Suggested fix: Remove line 74 (`using namespace std;`) from device.h entirely.

### High item #2 : `report_device_stats` never writes the Link-name column; all stats shifted one column

Location: src/devices/ahci.cpp : 307–325
Description: The table built in `ahci_create_device_stats_table` has five columns: "Link", "Active", "Partial", "Slumber", "Devslp". For row `idx`, the first cell occupies position `idx*5+5` (the "Link" column). `report_device_stats` starts writing at `offset = idx*5+5` and stores `active_util`, then `partial_util`, `slumber_util`, `devslp_util` — four values. Consequently: (1) the "Link" column for every row shows the active-utilization number instead of the device name; (2) "Active" shows partial utilisation; "Partial" shows slumber; "Slumber" shows devslp; and (3) the "Devslp" column is always empty. The human-readable link/disk name is never stored in the table at all.
Severity rationale: This is a logic error that always produces a visibly wrong HTML report for any system with AHCI links — a user-facing functional defect that always fires.
Suggested fix: Prepend a store of the link name before the utilisation values:
```cpp
void ahci::report_device_stats(std::vector<std::string> &ahci_data, int idx)
{
    int offset = (idx * 5 + 5);
    ahci_data[offset] = humanname;   // Link column
    offset += 1;
    ahci_data[offset] = std::format("{:5.1f}", get_result_value(active_rindex, &all_results));
    offset += 1;
    ahci_data[offset] = std::format("{:5.1f}", get_result_value(partial_rindex, &all_results));
    offset += 1;
    ahci_data[offset] = std::format("{:5.1f}", get_result_value(slumber_rindex, &all_results));
    offset += 1;
    ahci_data[offset] = std::format("{:5.1f}", get_result_value(devslp_rindex, &all_results));
}
```

### High item #3 : `closedir` not called before early return in `model_name`

Location: src/devices/ahci.cpp : 98
Description: Inside the `model_name` function, when a directory entry whose name contains both ':' and "target" is found, the code does `return disk_name(pathname, d_name, shortname);` immediately, bypassing the `closedir(dir)` call at line 101. Every call to `model_name` that successfully finds a matching entry leaks one open directory file descriptor. Because `create_all_ahcis` calls `ahci::ahci` which calls `model_name` for every AHCI device at start-up, each AHCI disk leaks one fd per measurement session start.
Severity rationale: File descriptor leak; bounded by number of AHCI devices but still a resource management defect.
Suggested fix:
```cpp
    std::string result = disk_name(pathname, d_name, shortname);
    closedir(dir);
    return result;
```
## Review of src/devices/devfreq.cpp, src/devices/devfreq.h, src/devices/alsa.h, src/devices/alsa.cpp, src/devices/runtime_pm.cpp — batch 24

### High item #1 : `using namespace std;` in alsa.cpp

Location: src/devices/alsa.cpp : 33
Description: `using namespace std;` appears at file scope in alsa.cpp. This is explicitly forbidden by both style.md ("Avoid `using namespace std;`") and general-c++.md ("`using namespace std;` is a deprecated construct"). All other files in this batch that need std names obtain them through the transitive pull of `device.h`'s own `using namespace std;`, which is itself a problem (see batch 23), but alsa.cpp adds a second independent violation. Pollutes the global namespace for all translation-unit consumers of this header chain.
Severity rationale: Explicitly named as forbidden in both style and language rules.
Suggested fix: Remove line 33 (`using namespace std;`). Add `std::` prefix to all unqualified standard-library names in the file (they are already there for most; the remaining uses of unqualified `string` on lines 44, 102, etc. are covered by device.h's `using namespace std;` leak, which should also be cleaned up separately).

### High item #2 : Unsigned underflow in `process_time_stamps()` loop bound

Location: src/devices/devfreq.cpp : 76
Description: `dstates.size()` returns `size_t` (unsigned). The expression `dstates.size()-1` wraps to `SIZE_MAX` when `dstates` is empty. The for-loop `for (i=0; i < dstates.size()-1; i++)` would then iterate from 0 to SIZE_MAX, repeatedly accessing `dstates[i]` out of bounds, causing undefined behaviour. After the loop, `dstates[i]` is accessed again at line 82 on the last element (the idle state). Under normal operation `start_measurement()` always inserts at least the idle state via `update_devfreq_freq_state(0,0)`, so this is not triggered at runtime today — but the code is one misuse or refactor away from silent memory corruption.
Severity rationale: Unsigned wrap-around producing an enormous loop bound and subsequent out-of-bounds vector access; undefined behaviour if triggered.
Suggested fix:
```cpp
if (dstates.empty())
    return;
for (i = 0; i < dstates.size() - 1; i++) { ... }
dstates[i]->time_after = sample_time - active_time;
```

### High item #3 : Division by zero in `fill_freq_utilization()` when `sample_time == 0`

Location: src/devices/devfreq.cpp : 196
Description: `fill_freq_utilization()` computes `percentage(1.0 * state->time_after / sample_time)`. `sample_time` is a `double` member initialised to 0 in `start_measurement()`. If `end_measurement()` has not been called yet, or if the two timestamps are identical (extremely short measurement window), `sample_time` is 0.0 and the division is undefined (IEEE 754 returns ±∞ or NaN). This value propagates into `percentage()` and eventually into the ncurses display as a garbage string.
Severity rationale: Results in NaN/infinity being formatted and displayed to the user; directly impacts user-visible output.
Suggested fix:
```cpp
if (sample_time <= 0.0)
    return "";
return std::format(" {:5.1f}% ", percentage(1.0 * state->time_after / sample_time));
```

## Review of runtime_pm.h, network.cpp, network.h, i915-gpu.h, i915-gpu.cpp — batch 25

### High item #1 : `using namespace std;` in network.cpp

Location: src/devices/network.cpp : 39
Description: `using namespace std;` is present at file scope. Per style.md §4.2 and general-c++.md, `using namespace std;` is an explicitly prohibited construct in this codebase. It pollutes the global namespace and can cause subtle name collisions. The rest of the file uses a mix of bare `string`, `map`, etc. (relying on the directive) alongside proper `std::` prefixed names.
Severity rationale: The style guide calls this a deprecated/prohibited construct; it is a systematic violation affecting the entire file.
Suggested fix: Remove `using namespace std;` and qualify all standard library names with `std::` (e.g., `std::map`, `std::string`, `string::npos` → `std::string::npos`).

### High item #2 : `using namespace std;` in i915-gpu.cpp

Location: src/devices/i915-gpu.cpp : 35
Description: Same violation as above — `using namespace std;` at file scope. Although the code in i915-gpu.cpp uses only a small amount of standard library functionality, the directive is present and constitutes a clear style violation as defined in style.md §4.2 and general-c++.md.
Severity rationale: Prohibited construct per project coding standards.
Suggested fix: Remove `using namespace std;`; add explicit `std::` prefixes where needed.

### High item #3 : Memory leak when `device_present()` returns false in `create_i915_gpu`

Location: src/devices/i915-gpu.cpp : 86–88
Description: A `gpu_rapl_device` object is allocated unconditionally (`rapl_dev = new class gpu_rapl_device(gpu)`), but it is only pushed onto `all_devices` if `rapl_dev->device_present()` returns true. When `device_present()` returns false the object is neither stored nor deleted, causing a permanent memory leak. Unlike the rest of PowerTOP where devices live for the duration of the process, this allocation happens early and the leaked object will never be reclaimed.
Severity rationale: Unconditional resource leak with no recovery path.
Suggested fix:
```cpp
rapl_dev = new(std::nothrow) gpu_rapl_device(gpu);
if (rapl_dev) {
    if (rapl_dev->device_present())
        all_devices.push_back(rapl_dev);
    else
        delete rapl_dev;
}
```
