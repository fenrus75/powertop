# PowerTOP

PowerTOP is a Linux tool used to diagnose issues with power consumption and
power management. In addition to being a diagnostic tool, PowerTOP also has an
interactive mode you can use to experiment with various power management
settings, for cases where the Linux distribution has not enabled those settings.

## Recent releases

| Version      | Highlights |
|--------------|------------|
| v2.16.1-rc1  | Build-system fixes: autoconf C++20 bump, missing Makefile.am files, legacy autoconf removed |
| v2.16        | AMD GPU tunable, CPU stats TUI redesign, Wayland-safe calibration, 14 new tunables |
| v2.16-rc3 | GPU tab visual polish: colour bars, combined idle bar, power profile display |
| v2.16-rc2 | Vertical scrollbar, terminal resize support, `--once` / `--auto-tune-dump` options, Meson build, C++20 |
| v2.15     | Previous stable release |

Full release notes: [`doc/relnotes.md`](doc/relnotes.md)

# Build dependencies

PowerTOP is written in C++ and targets Linux. It requires the
[Meson build system](https://mesonbuild.com/) and the following libraries:

* `ncurses` (required)
* `libnl-3` (required)
* `libtracefs` (required)
* `libtraceevent` (required)
* `libpci` (optional — only needed if the system has PCI devices)
* `gettext` (optional — for translated messages)

Example packages to install on Ubuntu:

    sudo apt install meson ninja-build libpci-dev libnl-3-dev libnl-genl-3-dev \
    libncursesw5-dev libtracefs-dev libtraceevent-dev gettext pkg-config


## Building PowerTOP

To build PowerTOP from the cloned source:

    meson setup build
    ninja -C build

To run the test suite:

    meson setup -Denable-tests=true build_test
    ninja -C build_test test


# Running PowerTOP

PowerTOP must be run as root. When invoked without arguments it starts in
interactive mode, which resembles `top`. Use the Tab and Shift-Tab keys to
navigate between the Overview, Idle stats, Frequency stats, Device stats,
Tunables, and WakeUp tabs.

Run `powertop --help` to see all options.


## Kernel configuration

PowerTOP needs some kernel configuration options enabled to function properly.
Modern distribution kernels typically have all of these enabled already. If you
are running a minimal or custom kernel, check for:

    CONFIG_NO_HZ
    CONFIG_HIGH_RES_TIMERS
    CONFIG_CPU_FREQ_GOV_ONDEMAND
    CONFIG_PERF_EVENTS
    CONFIG_TRACEPOINTS
    CONFIG_TRACING
    CONFIG_DEBUG_FS
    CONFIG_POWERCAP
    CONFIG_INTEL_RAPL    # Intel systems only


## Generating reports

PowerTOP supports three report formats. All report modes take one measurement
cycle (controlled by `--time`) and then write the report and exit.

**HTML** — rich report suitable for sharing:

    powertop --html                       # writes powertop.html
    powertop --html=myreport.html         # writes myreport.html

**Markdown** — plain-text report suitable for viewing in any Markdown renderer:

    powertop --markdown                   # writes powertop.md
    powertop --markdown=myreport.md       # writes myreport.md

**CSV** — machine-readable report:

    powertop --csv                        # writes powertop.csv
    powertop --csv=myreport.csv           # writes myreport.csv

Control how long PowerTOP measures before writing the report:

    powertop --html --time=60             # measure for 60 seconds, then report

Run multiple measurement iterations (useful for averaging):

    powertop --html --iteration=5 --time=20   # 5 iterations × 20 s each

Run a workload while measuring:

    powertop --html --workload=/path/to/script.sh

**Important:** Because PowerTOP is intended for privileged (`root`) use, reports
— especially those generated with `--debug` — will contain verbose system
information. PowerTOP **does not** sanitize or anonymize its reports. Be mindful
of this when sharing them.


## Filing a bug report

Generate and share a `--debug` HTML report:

    powertop --debug --html=bugreport.html

Then file the issue at: https://github.com/fenrus75/powertop/issues


## Automated tuning

PowerTOP can apply all recommended power-saving settings in one shot without
entering interactive mode:

    sudo powertop --auto-tune

To preview what changes would be made without actually applying them:

    sudo powertop --auto-tune-dump

The output of `--auto-tune-dump` is a list of shell commands. You can redirect
it to a script to apply selectively or at boot time:

    sudo powertop --auto-tune-dump > tune.sh
    # review tune.sh, then:
    sudo bash tune.sh


## Calibrating power estimates

PowerTOP, when running on battery, tracks power consumption and activity to
build up per-device power estimates. You can improve the accuracy of these
estimates by running a calibration cycle at least once:

    powertop --calibrate

Calibration cycles through various display brightness levels (including "off"),
USB device activities, and other workloads. Run it on battery with no other
active workload for best results.


## Other useful options

| Option | Description |
|--------|-------------|
| `--time=N` | Collect data for N seconds per iteration (default: 20) |
| `--iteration=N` | Run N measurement iterations (default: 1) |
| `--sample=N` | Power consumption sampling interval in seconds (default: 5) |
| `--workload=FILE` | Execute FILE as a workload during measurement |
| `--once` | Run one measurement cycle and exit (non-interactive) |
| `--quiet` | Suppress stderr output |
| `--debug` | Enable debug-level output |
| `--version` | Print version information |


## Extech Power Analyzer / Datalogger support

PowerTOP supports the Extech Power Analyzer/Datalogger (model 380803) over a
serial connection. Pass the device node on the command line:

    powertop --extech=/dev/ttyUSB0

(where `ttyUSB0` is the device node of the serial-to-USB adapter)


## HTML/CSS validation

If you make changes to the HTML reporting code, validate the output using:
* https://validator.w3.org/#validate_by_upload
* https://jigsaw.w3.org/css-validator/#validate_by_upload


# Contributing to PowerTOP and getting support

There are numerous ways to contribute to PowerTOP. See `CONTRIBUTE.md` for
details. The short version: fork the repository and send pull requests.

To file a bug or feature request:
* https://github.com/fenrus75/powertop/issues


# Code from other open source projects

PowerTOP contains code from other open source projects; we thank the authors
for their work. Specifically, PowerTOP contains code from:

```
Parse Event Library - Copyright 2009, 2010 Red Hat Inc  Steven Rostedt <srostedt@redhat.com>
nl80211 userspace tool - Copyright 2007, 2008	Johannes Berg <johannes@sipsolutions.net>
```


# Copyright and License

    PowerTOP
    Copyright (C) 2024  Intel Corporation

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

See `COPYING` file for a copy of the aforementioned (GPLv2) license.


## SPDX Tag

    /* SPDX-License-Identifier: GPL-2.0-only */

From: https://spdx.org/licenses/GPL-2.0-only.html
