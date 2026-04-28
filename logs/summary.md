# PowerTOP Code Review — Summary

## Batch 01 — src/calibrate/calibrate.cpp, src/calibrate/calibrate.h, src/lib.cpp, src/devlist.cpp, src/main.cpp
- Critical: 0  High: 5  Medium: 10  Low: 7  Nit: 5

## Batch 02 — src/test_framework.h, src/test_framework.cpp, src/display.cpp, src/display.h, src/lib.h
- Critical: 0  High: 4  Medium: 11  Low: 7  Nit: 7

## Batch 03 — src/devlist.h, src/tuning/tunable.cpp, src/tuning/tunable.h, src/tuning/runtime.h, src/tuning/runtime.cpp
- Critical: 0  High: 3  Medium: 7  Low: 7  Nit: 7

## Batch 04 — src/tuning/ethernet.cpp, src/tuning/ethernet.h, src/tuning/wifi.cpp, src/tuning/wifi.h, src/tuning/tuningusb.h
- Critical: 0  High: 6  Medium: 6  Low: 5  Nit: 4

## Batch 05 — src/tuning/tuningusb.cpp, src/tuning/tuning.cpp, src/tuning/tuning.h, src/tuning/bluetooth.cpp, src/tuning/bluetooth.h
- Critical: 0  High: 3  Medium: 6  Low: 8  Nit: 6

## Batch 06 — src/tuning/tuningsysfs.cpp, src/tuning/tuningsysfs.h, src/tuning/nl80211.h, src/tuning/iw.h, src/tuning/iw.c
- Critical: 0  High: 3  Medium: 5  Low: 8  Nit: 3

## Batch 07 — src/tuning/tuningi2c.h, src/tuning/tuningi2c.cpp, src/process/powerconsumer.h, src/process/powerconsumer.cpp, src/process/processdevice.h
- Critical: 0  High: 1  Medium: 8  Low: 7  Nit: 4

## Batch 08 — src/process/processdevice.cpp, src/process/process.h, src/process/process.cpp, src/process/work.cpp, src/process/work.h
- Critical: 0  High: 2  Medium: 8  Low: 4  Nit: 4
## Batch 09 — src/process/interrupt.h, src/process/interrupt.cpp, src/process/timer.cpp, src/process/timer.h, src/process/do_process.cpp
- Critical: 2  High: 7  Medium: 8  Low: 6  Nit: 4

## Batch 10 — src/perf/perf.h, src/perf/perf_event.h, src/perf/perf_bundle.cpp, src/perf/perf_bundle.h, src/perf/perf.cpp
- Critical: 3  High: 3  Medium: 9  Low: 7  Nit: 10

## Batch 11 — src/parameters/persistent.cpp, src/parameters/parameters.cpp, src/parameters/parameters.h, src/parameters/learn.cpp, src/report/report-formatter.h
- Critical: 0  High: 5  Medium: 6  Low: 5  Nit: 4

## Batch 12 — src/report/report-formatter-html.h, src/report/report.cpp, src/report/report.h, src/report/report-formatter-csv.h, src/report/report-maker.cpp
- Critical: 0  High: 3  Medium: 5  Low: 4  Nit: 2
## Batch 13 — src/report/report-maker.h, src/report/report-data-html.cpp, src/report/report-data-html.h, src/report/report-formatter-base.cpp, src/report/report-formatter-base.h
- Critical: 0  High: 2  Medium: 7  Low: 3  Nit: 4
## Batch 14 — src/report/report-formatter-html.cpp, src/report/report-formatter-csv.cpp, src/cpu/dram_rapl_device.cpp, src/cpu/dram_rapl_device.h, src/cpu/cpu_linux.cpp
- Critical: 0  High: 2  Medium: 4  Low: 7  Nit: 3
## Batch 15 — src/cpu/cpu_core.cpp, src/cpu/cpudevice.h, src/cpu/cpudevice.cpp, src/cpu/abstract_cpu.cpp, src/cpu/cpu_rapl_device.cpp
- Critical: 0  High: 2  Medium: 3  Low: 5  Nit: 5
## Batch 16 — src/cpu/cpu_rapl_device.h, src/cpu/cpu_package.cpp, src/cpu/cpu.h, src/cpu/cpu.cpp, src/cpu/intel_cpus.cpp
- Critical: 1  High: 4  Medium: 8  Low: 5  Nit: 3
## Batch 17 — src/cpu/intel_cpus.h, src/cpu/intel_gpu.cpp, src/cpu/rapl/rapl_interface.cpp, src/cpu/rapl/rapl_interface.h, src/wakeup/wakeup.h
- Critical: 0  High: 2  Medium: 6  Low: 6  Nit: 4
## Batch 18 — src/wakeup/wakeup_usb.h, src/wakeup/wakeup_ethernet.h, src/wakeup/waketab.cpp, src/wakeup/wakeup_ethernet.cpp, src/wakeup/wakeup.cpp
- Critical: 0  High: 3  Medium: 3  Low: 5  Nit: 4
## Batch 19 — src/wakeup/wakeup_usb.cpp, src/measurement/acpi.cpp, src/measurement/acpi.h, src/measurement/opal-sensors.cpp, src/measurement/opal-sensors.h
- Critical: 0  High: 1  Medium: 2  Low: 4  Nit: 7
## Batch 20 — src/measurement/sysfs.h, src/measurement/sysfs.cpp, src/measurement/measurement.h, src/measurement/measurement.cpp, src/measurement/extech.h
- Critical: 0  High: 3  Medium: 5  Low: 4  Nit: 5
## Batch 21 — src/measurement/extech.cpp, src/devices/backlight.h, src/devices/backlight.cpp, src/devices/rfkill.h, src/devices/rfkill.cpp
- Critical: 1  High: 3  Medium: 7  Low: 5  Nit: 5
## Batch 22 — src/devices/usb.h, src/devices/usb.cpp, src/devices/gpu_rapl_device.h, src/devices/gpu_rapl_device.cpp, src/devices/ahci.h
- Critical: 0  High: 2  Medium: 6  Low: 7  Nit: 3
## Batch 23 — src/devices/ahci.cpp, src/devices/thinkpad-fan.h, src/devices/thinkpad-fan.cpp, src/devices/device.h, src/devices/device.cpp
- Critical: 0  High: 3  Medium: 8  Low: 6  Nit: 5
## Batch 24 — src/devices/devfreq.cpp, src/devices/devfreq.h, src/devices/alsa.h, src/devices/alsa.cpp, src/devices/runtime_pm.cpp
- Critical: 0  High: 3  Medium: 8  Low: 6  Nit: 4
## Batch 25 — src/devices/runtime_pm.h, src/devices/network.cpp, src/devices/network.h, src/devices/i915-gpu.h, src/devices/i915-gpu.cpp
- Critical: 0  High: 3  Medium: 6  Low: 9  Nit: 7
## Batch 26 — src/devices/thinkpad-light.cpp, src/devices/thinkpad-light.h
- Critical: 0  High: 0  Medium: 3  Low: 4  Nit: 4

## GRAND TOTAL (all 26 batches)
- Critical: 7  High: 78  Medium: 165  Low: 151  Nit: 123
