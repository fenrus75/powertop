# PowerTOP AI Tooling Reference

This document describes the utilities available for managing the PowerTOP Test Framework. These tools are designed to facilitate the creation, modification, and analysis of deterministic system traces.

## trace_tool.py

**Location:** `scripts/test_tools/trace_tool.py`  
**Purpose:** Manipulation and analysis of `.ptrecord` (also `.trace`) files used by the PowerTOP test framework.

### Trace File Format Spec
The trace file is a line-based text format. Each line represents a single I/O operation:

| Type | Format | Description |
|:---|:---|:---|
| `R` | `R <path> <b64>` | Sysfs/proc **read** — base64-encoded file content |
| `W` | `W <path> <b64>` | Sysfs **write** — base64-encoded value written |
| `N` | `N <path>` | **Miss** — file not found (no content) |
| `M` | `M <cpu> <hex_offset> <hex_value>` | **MSR read** — no trailing b64 |
| `T` | `T <tv_sec> <tv_usec>` | **Time** snapshot — no trailing b64 |
| `L` | `L <b64> <path>` | **Symlink** read (`pt_readlink`) — **b64 comes first**, path second. Empty b64 (`L  <path>`) means the readlink failed. Path may contain spaces. |

> **Note:** The `L` record layout is reversed from `R`/`W`: the base64 token comes *before* the path, because symlink target paths may contain spaces.

### Build Configuration
To use the test framework, you must build PowerTOP with the test framework enabled.

```bash
meson setup build_tf -Denable-tests=true
ninja -C build_tf
```

---

## Recording a Session

Recording captures real system data (files read from `/proc`, `/sys`, etc.) and the writes PowerTOP attempts to perform. **Recording requires root privileges** because it accesses restricted system files.

### Examples
**1. Standard recording:**
```bash
sudo ./powertop --record=full_session.trace
```

**2. Record a specific workload for a set duration:**
```bash
sudo ./powertop --time=30 --once --record=workload.trace
```

---

## Replaying a Session

Replaying uses the recorded trace instead of the live system. **Replay does not require root privileges** and will not modify any actual system files (writes are validated against the trace but not executed).

### Examples
**1. Basic replay:**
```bash
./powertop --replay=full_session.trace
```

**2. Replay with CSV report generation:**
```bash
./powertop --replay=full_session.trace --csv=report.csv
```

---

### Command Reference

| Command | Usage | Description |
|:---|:---|:---|
| **list** | `list <trace> [-c]` | List all entries. `-c`/`--content` shows decoded values inline. |
| **search** | `search <trace> <regex>` | Find entries whose path matches a regex. |
| **grep** | `grep <trace> <string>` | Find entries whose decoded content contains a string. |
| **extract** | `extract <trace> <line> <out>` | Write the decoded content of one entry to a file. For `L` records, writes the symlink target. |
| **replace** | `replace <trace> <line> <in>` | Replace the content of one entry from a file. Handles `L` records (replaces target). |
| **edit** | `edit <trace> <line>` | Open one entry's content in `$EDITOR` for interactive editing. |
| **export** | `export <trace> <dir>` | Dump all decoded entries to a directory for bulk inspection. |
| **validate** | `validate <trace>` | Check all record types and base64 encoding for correctness. |
| **add** | `add <trace> <type> <path> [value]` | **Append a new record** to a trace file, creating it if needed. See below. |

### `add` Command Details

`add` is the primary tool for building fixture files from scratch without manually computing base64.

```bash
# Append a sysfs read returning "s2idle"
trace_tool.py add my.ptrecord R /sys/power/mem_sleep "s2idle"

# Append a write expectation
trace_tool.py add my.ptrecord W /sys/power/mem_sleep "deep"

# Append a file-not-found (N) entry
trace_tool.py add my.ptrecord N /sys/class/net/eth0/device/driver

# Append a symlink read (L) with a target
trace_tool.py add my.ptrecord L /sys/class/rfkill/rfkill0/device/driver \
    "/sys/bus/pci/drivers/ath9k_htc"

# Append a broken symlink (empty target = readlink failed)
trace_tool.py add my.ptrecord L /sys/class/rfkill/rfkill0/device/device/driver
```

The file is **created** if it does not already exist. Records are appended in order; the test framework replays them in the exact order they appear.

---

### Agent Workflow: Creating a Fixture from Scratch

Use `add` to build a `.ptrecord` file record by record, matching the exact read sequence of the class under test:

1. **Identify the read sequence** by reading the constructor and `start_measurement`/`end_measurement` source.
2. **Add reads in order** using `trace_tool.py add`:
   - One `R` per `read_sysfs_string` or `read_sysfs` call (with the value the file would return).
   - One `N` for any path that should appear absent.
   - One `L` per `pt_readlink` call (with the target, or no target for a broken link).
3. **Verify** with `trace_tool.py validate my.ptrecord`.
4. **Inspect** with `trace_tool.py list my.ptrecord --content` to confirm the sequence looks correct.

### Agent Workflow: Modifying a Recorded Session

1.  **Record:** Run `powertop --record=my.trace` on a live system.
2.  **Filter:** Use `trace_tool.py search my.trace "/sys/devices/..."` to find the hardware state you want to simulate.
3.  **Inspect:** Use `trace_tool.py list my.trace --content` or `trace_tool.py extract` to see what PowerTOP read.
4.  **Mock:** Use `trace_tool.py replace` or `trace_tool.py edit` to inject a different value.
5.  **Verify:** Run the relevant unit test or `powertop --replay=my.trace` to confirm correct behaviour.

