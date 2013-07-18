AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I ../m4 

sbin_PROGRAMS = powertop
nodist_powertop_SOURCES = css.h

powertop_SOURCES = parameters/persistent.cpp parameters/learn.cpp parameters/parameters.cpp \
		parameters/parameters.h display.cpp cpu/cpu_core.cpp cpu/cpudevice.cpp  \
		cpu/cpu.cpp cpu/cpu.h cpu/cpu_linux.cpp cpu/abstract_cpu.cpp cpu/intel_cpus.h \
		cpu/cpu_package.cpp cpu/cpudevice.h cpu/intel_cpus.cpp devlist.h devlist.cpp \
		lib.cpp report/report.cpp process/process.cpp process/interrupt.h process/timer.h  \
		process/processdevice.cpp process/powerconsumer.cpp process/work.h \
		process/timer.cpp process/process.h process/work.cpp process/do_process.cpp \
		process/processdevice.h process/interrupt.cpp process/powerconsumer.h report/report.h \
		perf/perf.cpp perf/perf_event.h perf/perf.h perf/perf_bundle.h perf/perf_bundle.cpp \
		display.h devices/rfkill.h devices/i915-gpu.cpp devices/device.cpp devices/alsa.cpp \
		devices/runtime_pm.cpp devices/thinkpad-fan.cpp devices/usb.cpp devices/ahci.cpp \
		devices/thinkpad-light.cpp devices/ahci.h devices/i915-gpu.h devices/network.h \
		devices/rfkill.cpp devices/alsa.h devices/thinkpad-fan.h devices/device.h \
		devices/usb.h devices/backlight.h devices/backlight.cpp devices/runtime_pm.h \
		devices/thinkpad-light.h devices/network.cpp lib.h tuning tuning/cpufreq.h \
		tuning/bluetooth.cpp tuning/cpufreq.cpp tuning/tuning.cpp tuning/tuningusb.cpp \
		tuning/ethernet.cpp tuning/bluetooth.h tuning/tuning.h tuning/ethernet.h \
		tuning/tunable.cpp tuning/nl80211.h tuning/iw.c tuning/wifi.cpp tuning/tuningsysfs.h \
		tuning/tuningsysfs.cpp tuning/wifi.h tuning/runtime.cpp tuning/tunable.h \
		tuning/runtime.h tuning/tuningusb.h tuning/iw.h calibrate/calibrate.cpp \
		calibrate/calibrate.h measurement/measurement.cpp measurement/power_supply.cpp \
		measurement/measurement.h measurement/acpi.cpp measurement/sysfs.h measurement/sysfs.cpp \
		measurement/acpi.h measurement/extech.cpp measurement/power_supply.h measurement/extech.h \
		report/report-maker.cpp report/report-maker.h report/report-formatter.h \
		report/report-formatter-base.cpp report/report-formatter-base.h \
		report/report-formatter-csv.cpp report/report-formatter-csv.h \
		report/report-formatter-html.cpp report/report-formatter-html.h \
		main.cpp css.h powertop.css cpu/intel_gpu.cpp \
		cpu/rapl/rapl_interface.cpp cpu/cpu_rapl_device.cpp cpu/rapl/rapl_interface.h\
		cpu/dram_rapl_device.cpp devices/gpu_rapl_device.cpp cpu/cpu_rapl_device.h \
		cpu/dram_rapl_device.h devices/gpu_rapl_device.h


powertop_CXXFLAGS = -fno-omit-frame-pointer -fstack-protector -Wall -Wshadow -Wformat $(NCURSES_CFLAGS) $(PCIUTILS_CFLAGS) $(LIBNL_CFLAGS) $(GLIB2_CFLAGS)


powertop_CPPFLAGS = -D_FORTIFY_SOURCE=2 $(NCURSES_CFLAGS) $(PCIUTILS_CFLAGS) $(LIBNL_CFLAGS) $(GLIB2_CFLAGS) $(LIBZ_CFLAGS) -DLOCALEDIR=\"$(localedir)\"

powertop_LDADD = ../traceevent/libtraceevnet.la

AM_LDFLAGS = $(LIBS) $(NCURSES_LIBS) $(PCIUTILS_LIBS) $(LIBNL_LIBS) $(LIBZ_LIBS) $(PTHREAD_LIBS)  $(RESOLV_LIBS)

BUILT_SOURCES = css.h
CLEANFILES = css.h
css.h: powertop.css
	$(SHELL) ./csstoh.sh powertop.css css.h

