AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I ../m4 

noinst_PROGRAMS = csstoh
csstoh_SOURCES = csstoh.c 

sbin_PROGRAMS = powertop
nodist_powertop_SOURCES = css.h

powertop_SOURCES = parameters/persistent.cpp parameters/learn.cpp parameters/parameters.cpp parameters/parameters.h display.cpp cpu/cpu_core.cpp cpu/cpudevice.cpp cpu/cpu.cpp cpu/cpu.h cpu/cpu_linux.cpp cpu/abstract_cpu.cpp cpu/intel_cpus.h cpu/cpu_package.cpp cpu/cpudevice.h cpu/intel_cpus.cpp devlist.h devlist.cpp lib.cpp report.cpp process/process.cpp process/interrupt.h process/timer.h process/processdevice.cpp process/powerconsumer.cpp process/work.h  process/timer.cpp process/process.h process/work.cpp process/do_process.cpp process/processdevice.h process/interrupt.cpp process/powerconsumer.h report.h perf/perf.cpp perf/perf_event.h perf/perf.h perf/perf_bundle.h perf/perf_bundle.cpp display.h devices/rfkill.h devices/i915-gpu.cpp devices/device.cpp devices/alsa.cpp devices/runtime_pm.cpp devices/thinkpad-fan.cpp devices/usb.cpp devices/ahci.cpp devices/thinkpad-light.cpp devices/ahci.h devices/i915-gpu.h devices/network.h devices/rfkill.cpp devices/alsa.h devices/thinkpad-fan.h devices/device.h devices/usb.h devices/backlight.h devices/backlight.cpp devices/runtime_pm.h devices/thinkpad-light.h devices/network.cpp lib.h tuning tuning/cpufreq.h tuning/bluetooth.cpp tuning/cpufreq.cpp tuning/tuning.cpp tuning/tuningusb.cpp tuning/ethernet.cpp tuning/bluetooth.h tuning/tuning.h tuning/ethernet.h tuning/tunable.cpp tuning/nl80211.h tuning/iw.c tuning/wifi.cpp tuning/tuningsysfs.h tuning/tuningsysfs.cpp tuning/wifi.h tuning/runtime.cpp tuning/tunable.h tuning/runtime.h tuning/tuningusb.h tuning/iw.h calibrate/calibrate.cpp calibrate/calibrate.h measurement/measurement.cpp measurement/power_supply.cpp measurement/measurement.h measurement/acpi.cpp measurement/sysfs.h measurement/sysfs.cpp measurement/acpi.h measurement/extech.cpp measurement/power_supply.h measurement/extech.h main.cpp css.h powertop.css


powertop_CXXFLAGS = -O2 -g -fno-omit-frame-pointer -fstack-protector -Wall -Wshadow -Wformat -I/usr/include/ $(NCURSES_CFLAGS) $(PCIUTILS_CFLAGS) $(LIBNL_CFLAGS) $(GLIB2_CFLAGS)


powertop_CPPFLAGS = -D_FORTIFY_SOURCE=2 $(NCURSES_CFLAGS) $(PCIUTILS_CFLAGS) $(LIBNL_CFLAGS) $(GLIB2_CFLAGS) $(LIBZ_CFLAGS)

powertop_LDADD = ../pevent/libparseevent.la

AM_LDFLAGS = $(LIBS) $(NCURSES_LIBS) $(PCIUTILS_LIBS) $(LIBNL_LIBS) $(LIBZ_LIBS) $(NCURES_LIBS) $(PTHREAD_LIBS)  $(RESOLV_LIBS)

BUILT_SOURCES = css.h
CLEANFILES = css.h
css.h: csstoh powertop.css
	./csstoh powertop.css css.h

