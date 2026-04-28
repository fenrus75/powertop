
## Review of interrupt.h, interrupt.cpp, timer.cpp, timer.h, do_process.cpp — batch 09

### Critical item #1 : change_blame accesses cpu_level/cpu_blame without bounds check

Location: src/process/do_process.cpp : 144
Description: `change_blame()` accesses `cpu_level[cpu]` and `cpu_blame[cpu]` directly without any bounds check. By contrast, `consume_blame()` (line 152) does check bounds before accessing the same vectors. Both vectors are resized in `process_process_data()` to `get_max_cpu()+1`, but a trace event may carry a CPU number larger than `get_max_cpu()` (e.g., due to CPU hotplug or late-arriving events). An out-of-bounds write to these vectors is undefined behaviour and will likely cause a crash or silent data corruption.
Severity rationale: Direct, unconditional write to a potentially out-of-bounds vector index; the inconsistency with `consume_blame` shows the missing guard is an oversight rather than intentional design.
Suggested fix:
```cpp
static void change_blame(unsigned int cpu, class power_consumer *consumer, int level)
{
    if (cpu_level.size() <= cpu || cpu_blame.size() <= cpu)
        return;
    if (cpu_level[cpu] >= level)
        return;
    cpu_blame[cpu] = consumer;
    cpu_level[cpu] = level;
}
```
Fixed-with: 52811c8

### Critical item #2 : perf_events dangling pointer after clear_process_data

Location: src/process/do_process.cpp : 1213
Description: `clear_process_data()` calls `delete perf_events` but never sets `perf_events = nullptr`. The variable is a file-scope static and retains the stale pointer value. If `start_process_measurement()` is subsequently called (e.g., on a second measurement run), the guard `if (!perf_events)` evaluates to *false* because the pointer is non-null, the initialisation block is skipped entirely, and `perf_events->start()` is invoked on the freed object — a classic use-after-free leading to undefined behaviour / crash.
Severity rationale: Directly causes a use-after-free on any second invocation of the measurement cycle, which is normal PowerTOP operation.
Suggested fix: Add `perf_events = nullptr;` immediately after `delete perf_events;` in `clear_process_data()`.
Fixed-with: 9d9b536

## Review of src/perf/perf.h, src/perf/perf_event.h, src/perf/perf_bundle.cpp, src/perf/perf_bundle.h, src/perf/perf.cpp — batch 10

### Critical item #1 : Null pointer dereference in `set_event_name` after `parse_event_format` succeeds

Location: src/perf/perf.cpp : 163
Description: In `set_event_name`, after `parse_event_format` returns success (ret >= 0), `tep_find_event_by_name` is called a second time. If it still returns NULL (e.g. the event was parsed but the name lookup fails), execution falls through to `trace_type = event->id` on the very next line with a NULL `event` pointer. No null check is present after the second call.
Severity rationale: This is a direct null pointer dereference that will crash the process whenever an event format is parseable but not findable by name — a realistic scenario for any unexpected trace event name.
Suggested fix:
```cpp
event = tep_find_event_by_name(perf_event::tep, system_name.c_str(), event_name.c_str());
if (!event) {
    trace_type = -1;
    return;
}
trace_type = event->id;
```
Fixed-with: e401353

Location: src/perf/perf.cpp : 231
Description: In `create_perf_event`, if `mmap()` fails the function returns early (line 122) without setting `pc` or `data_mmap`. However `perf_fd` has already been set to a valid file descriptor. The guard in `process()` only checks `if (perf_fd < 0)`, so when mmap has failed but `perf_fd` is valid, execution proceeds to `pc->data_tail` where `pc` is NULL (set only in the constructors, not explicitly initialized to NULL there either). This is an unconditional null pointer dereference.
Severity rationale: Any call to `process()` after a mmap failure will crash the process. mmap failures are realistic (e.g. under low memory or file-descriptor pressure).
Suggested fix: On mmap failure in `create_perf_event`, close and reset `perf_fd` to -1 before returning, so the guard in `process()` will correctly skip processing. Also initialize `pc = nullptr` and `data_mmap = nullptr` in both constructors.
Fixed-with: 9bed143

### Critical item #3 : `malloc` return value unchecked before `memcpy` in `handle_event`

Location: src/perf/perf_bundle.cpp : 63
Description: `malloc(header->size)` is called and its return value is immediately passed to `memcpy` without any null check. If the allocation fails, `buffer` is NULL and `memcpy(NULL, ...)` is undefined behavior and will crash the process.
Severity rationale: `malloc` is documented to return NULL on allocation failure. Writing to a NULL pointer is undefined behavior and will cause a segfault in practice.
Suggested fix:
```cpp
buffer = (unsigned char *)malloc(header->size);
if (!buffer) {
    fprintf(stderr, _("Out of memory in perf handle_event\n"));
    return;
}
memcpy(buffer, header, header->size);
```

## Review of src/cpu/cpu_rapl_device.h, src/cpu/cpu_package.cpp, src/cpu/cpu.h, src/cpu/cpu.cpp, src/cpu/intel_cpus.cpp — batch 16

### Critical item #1 : Null pointer dereference of `cpu` in handle_trace_point

Location: src/cpu/cpu.cpp : 953
Description: In `perf_power_bundle::handle_trace_point`, after the upper-bounds check
`if (cpunr >= (int)all_cpus.size())`, the code assigns `cpu = all_cpus[cpunr]`.
The `all_cpus` vector is populated with explicit NULL sentinels when resized
(`all_cpus.resize(number + 1, NULL)`), so any slot not filled by `handle_one_cpu`
(e.g., due to non-contiguous CPU numbering, CPU hot-unplug, or late
enumeration) will be NULL. `cpu` is then used unconditionally at lines 973, 975,
987, 991, and 993 (`cpu->go_unidle`, `cpu->go_idle`, `cpu->change_freq`) with no
null check, causing an immediate null pointer dereference and crash whenever a
perf event arrives for such a slot.
Severity rationale: This directly and materially crashes powertop on systems
with non-contiguous CPU numbers or after CPU hot-unplug, which are common on
server platforms and any machine that has ever used CPU hotplug.
Suggested fix:
```cpp
cpu = all_cpus[cpunr];
if (!cpu)
    return;
```

## Review of src/measurement/extech.cpp — batch 21

### Critical item #1 : parse_packet validation loop uses `i * 0` instead of `i * 5`

Location: src/measurement/extech.cpp : 216
Description: The outer validation loop in `parse_packet` iterates `i` from 0 to 3, but uses `i * 0` as the index, which is always 0. Consequently all four iterations check only `buf[0] != 2` and `buf[0 + 4] != 3`, never validating the 2nd, 3rd, or 4th five-byte packet. Any corrupt or truncated packet that has valid bytes at positions 0 and 4 will silently pass validation. The intended expression is `i * 5` and `i * 5 + 4`.
Severity rationale: The validation function is completely broken for all but the first packet block. Malformed packets can pass undetected, leading to incorrect power readings being reported to users — this directly and materially impacts the tool's core measurement function.
Suggested fix: Change `p->buf[i * 0]` to `p->buf[i * 5]` and `p->buf[i * 0 + 4]` to `p->buf[i * 5 + 4]`.
