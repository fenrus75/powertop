AUTOMAKE_OPTIONS = subdir-objects

sbin_PROGRAMS = powertop
nodist_powertop_SOURCES = css.h

powertop_SOURCES = \
	css.h \
	devlist.cpp \
	devlist.h \
	display.cpp \
	display.h \
	lib.cpp \
	lib.h \
	main.cpp \
	powertop.css \
	\
	calibrate/calibrate.cpp \
	calibrate/calibrate.h \
	cpu/abstract_cpu.cpp \
	cpu/cpu.cpp \
	cpu/cpu.h \
	cpu/cpu_core.cpp \
	cpu/cpu_linux.cpp \
	cpu/cpu_package.cpp \
	cpu/cpu_rapl_device.cpp \
	cpu/cpu_rapl_device.h \
	cpu/cpudevice.cpp \
	cpu/cpudevice.h \
	cpu/dram_rapl_device.cpp \
	cpu/dram_rapl_device.h \
	cpu/intel_cpus.cpp \
	cpu/intel_cpus.h \
	cpu/intel_gpu.cpp \
	cpu/rapl/rapl_interface.cpp \
	cpu/rapl/rapl_interface.h \
	devices/ahci.cpp \
	devices/ahci.h \
	devices/alsa.cpp \
	devices/alsa.h \
	devices/backlight.cpp \
	devices/backlight.h \
	devices/devfreq.cpp \
	devices/devfreq.h \
	devices/device.cpp \
	devices/device.h \
	devices/gpu_rapl_device.cpp \
	devices/gpu_rapl_device.h \
	devices/i915-gpu.cpp \
	devices/i915-gpu.h \
	devices/network.cpp \
	devices/network.h \
	devices/rfkill.cpp \
	devices/rfkill.h \
	devices/runtime_pm.cpp \
	devices/runtime_pm.h \
	devices/thinkpad-fan.cpp \
	devices/thinkpad-fan.h \
	devices/thinkpad-light.cpp \
	devices/thinkpad-light.h \
	devices/usb.cpp \
	devices/usb.h \
	measurement/acpi.cpp \
	measurement/acpi.h \
	measurement/extech.cpp \
	measurement/extech.h \
	measurement/measurement.cpp \
	measurement/measurement.h \
	measurement/sysfs.cpp \
	measurement/sysfs.h \
	parameters/learn.cpp \
	parameters/parameters.cpp \
	parameters/parameters.h \
	parameters/persistent.cpp \
	perf/perf.cpp \
	perf/perf.h \
	perf/perf_bundle.cpp \
	perf/perf_bundle.h \
	perf/perf_event.h \
	process/do_process.cpp \
	process/interrupt.cpp \
	process/interrupt.h \
	process/powerconsumer.cpp \
	process/powerconsumer.h \
	process/process.cpp \
	process/process.h \
	process/processdevice.cpp \
	process/processdevice.h \
	process/timer.cpp \
	process/timer.h \
	process/work.cpp \
	process/work.h \
	report/report-data-html.cpp \
	report/report-data-html.h \
	report/report-formatter-base.cpp \
	report/report-formatter-base.h \
	report/report-formatter-csv.cpp \
	report/report-formatter-csv.h \
	report/report-formatter-html.cpp \
	report/report-formatter-html.h \
	report/report-formatter.h \
	report/report-maker.cpp \
	report/report-maker.h \
	report/report.cpp \
	report/report.h \
	tuning/bluetooth.cpp \
	tuning/bluetooth.h \
	tuning/ethernet.cpp \
	tuning/ethernet.h \
	tuning/iw.c \
	tuning/iw.h \
	tuning/nl80211.h \
	tuning/runtime.cpp \
	tuning/runtime.h \
	tuning/tunable.cpp \
	tuning/tunable.h \
	tuning/tuning.cpp \
	tuning/tuning.h \
	tuning/tuningsysfs.cpp \
	tuning/tuningsysfs.h \
	tuning/tuningusb.cpp \
	tuning/tuningusb.h \
	tuning/tuningi2c.cpp \
	tuning/tuningi2c.h \
	tuning/wifi.cpp \
	tuning/wifi.h

powertop_CXXFLAGS = \
	-Wall \
	-Wformat \
	-Wshadow \
	-fno-omit-frame-pointer \
	-fstack-protector \
	$(GLIB2_CFLAGS) \
	$(LIBNL_CFLAGS) \
	$(NCURSES_CFLAGS) \
	$(PCIUTILS_CFLAGS)


powertop_CPPFLAGS = \
	-DLOCALEDIR=\"$(localedir)\" \
	$(GLIB2_CFLAGS) \
	$(LIBNL_CFLAGS) \
	$(LIBZ_CFLAGS) \
	$(NCURSES_CFLAGS) \
	$(PCIUTILS_CFLAGS)

powertop_LDADD = \
	../traceevent/libtraceevnet.la

AM_LDFLAGS = \
	$(LIBNL_LIBS) \
	$(LIBS) \
	$(LIBZ_LIBS) \
	$(NCURSES_LIBS) \
	$(PCIUTILS_LIBS) \
	$(PTHREAD_LIBS) \
	$(RESOLV_LIBS)

BUILT_SOURCES = css.h
CLEANFILES = css.h

css.h: powertop.css
	$(SHELL) ${srcdir}/csstoh.sh ${srcdir}/powertop.css css.h

EXTRA_DIST = ${srcdir}/csstoh.sh
