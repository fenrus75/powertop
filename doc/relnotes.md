# PowerTOP Release Notes

## v2.16.1-rc1

### User-visible enhancements and changes

This is a build-system focused maintenance release; there are no new
tunables, UI changes, or user-facing features in this cycle.

- Fixed build failures on distributions with newer autotools toolchains by
  bumping the autoconf C++ standard requirement to C++20, matching the
  Meson build
- Added previously-missing source files to `src/Makefile.am` so the
  (legacy) autoconf build no longer fails to link on some configurations
- Removed the legacy autoconf build support entirely (Meson is now the
  sole supported build system), eliminating a class of build breakage
  reports caused by the two build systems drifting out of sync

### New command-line options

None in this cycle.

### Internal changes

Removed `autogen.sh` and `configure.ac`; Meson is now the only supported
build system, which removes the maintenance burden of keeping two build
definitions in sync.

### Thanks

We thank the following external contributors who submitted patches for this release:

- Michael Vetter

## v2.16

### User-visible enhancements and changes

**New tunables**

The following sysfs tunables have been added (sourced from the
`clr-power-tweaks` curated list):

| Tunable | sysfs path | Suggested value |
|---|---|---|
| VM dirty ratio | `/proc/sys/vm/dirty_ratio` | `50` |
| Transparent hugepage scan sleep | `/sys/kernel/mm/transparent_hugepage/khugepaged/scan_sleep_millisecs` | `30000` |
| Framebuffer cursor blink | `/sys/devices/virtual/graphics/fbcon/cursor_blink` | `0` |
| KSM sleep interval | `/sys/kernel/mm/ksm/sleep_millisecs` | `4000` |
| KSM batch size | `/sys/kernel/mm/ksm/pages_to_scan` | `1000` |
| Autogroup scheduling | `/proc/sys/kernel/sched_autogroup_enabled` | `0` |
| Intel P-state minimum performance | `/sys/devices/system/cpu/intel_pstate/min_perf_pct` | `50` |
| Network default qdisc | `/proc/sys/net/core/default_qdisc` | `fq` |
| Intel ITMT (Turbo Boost Max 3.0) | `/proc/sys/kernel/sched_itmt_enabled` | `1` |
| Intel energy performance bias | `/sys/devices/system/cpu/cpu*/power/energy_perf_bias` | `balance-performance` |
| Intel energy performance preference | `/sys/devices/system/cpu/cpufreq/policy*/energy_performance_preference` | `balance_performance` |
| Intel Xe GPU power profile | `/sys/class/drm/card*/device/tile*/gt*/freq0/power_profile` | `power_saving` |
| Enable Audio codec power management | `/sys/module/snd_hda_intel/parameters/power_save` | `1` |
| AMD GPU panel power savings | `/sys/class/drm/*/amdgpu/panel_power_savings` | `2` |

Wildcard sysfs paths (e.g. `cpu*`) are now handled natively — PowerTOP
applies the tunable to all matching sysfs files and reports Good only when
all of them are already at the suggested value.

**Display / UI**
- GPU tab (Intel Xe): progress bar scale now always shows the true maximum
  value right-aligned, suppressing any overlapping tick label
- GPU tab: frequency bars now start at 0 instead of the hardware minimum,
  giving a correct visual baseline
- GPU tab: replaced `>` / `<` policy markers with a four-segment colour bar
  (bold green = floor, green = active, yellow = headroom, dim = beyond policy)
- GPU tab: C0/C6 idle bars merged into a single two-colour bar
  (red = busy/C0, bright green = idle/C6) since C0 is derived from C6
- GPU tab: active xe power profile displayed on the Power Overview section
- GPU tab: only shown on systems with Intel Xe GPU and xe hwmon present
- CPU Frequency Stats tab: redesigned with progress bars, corrected bar
  width and spacing, and fixed C0 active column misalignment in Idle stats
- CPU Frequency Stats tab: empty cores are skipped in display
- Show "Preparing to take measurements" message during the initial
  one-second measurement so the screen is no longer blank at startup
- Vertical scrollbar added to tab content panes
- Scroll-down capped at content extent to prevent over-scrolling
- Terminal resize (KEY_RESIZE / SIGWINCH) now handled correctly while
  PowerTOP is running
- Notification system redesigned to use the status bar instead of
  overlaying content
- Tab bar and bottom line now correctly fill the full terminal width

**Reports**
- HTML and CSV output: fixed escaping and i18n issues
- Markdown report output format added (`--output=file.md`)

**Correctness / stability fixes**
- `--calibrate`: display power control now uses sysfs DRM DPMS directly,
  falling back to xset only if needed — works on X11, Wayland, and headless
- Fixed three memory leaks found by Valgrind (power meters, tuning
  window, wakeup tab window)
- Fixed double-free in devfreq cleanup
- Fixed sysfs residency overflow in Intel GPU reporting
- Fixed RAPL division-by-zero and improved error handling
- Fixed Bluetooth byte-counter logic and removed deprecated tooling
- Added near-zero division guards for AHCI, ALSA, and RAPL power
  calculations
- Fixed `--batch` mode numeric safety and i18n issues
- Fixed halfdelay timeout clamping to valid ncurses range
- Fixed null pointer dereference when accessing first CPU in some configs
- Fixed potential memory leak in GPU handler initialisation

**Translations**
- 17 previously missing source files added to translation coverage
- All `.po` files regenerated; format-specifier changes (`%s` → `{}`)
  applied mechanically across all 11 languages

### New command-line options

- `--once`: take a single measurement and exit
- `--auto-tune-dump`: run auto-tune and dump results without
  interactive display

### Thanks

We thank the following external contributors who submitted patches for this release:

- Anthony Ruhier
- George Burgess IV
- Jaroslav Škarvada
- Johannes Truschnigg
- Pascal Nasahl
- Renat Sabitov
- rottenpants466
- Simon Howard
- Steven Rostedt
- vallode
- Zentaro Kavanagh
