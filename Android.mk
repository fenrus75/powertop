LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := debug
LOCAL_SHARED_LIBRARIES := libstlport \
			  libnl \
			  libpci \
			  libtraceevnet \
LOCAL_MODULE := powertop  

#LOCAL_CFLAGS += -Wall -O2 -g -fno-omit-frame-pointer -fstack-protector -Wshadow -Wformat -D_FORTIFY_SOURCE=2
#LOCAL_CPPFLAGS += -Wall -O2 -g -fno-omit-frame-pointer

LOCAL_C_INCLUDES += external/stlport/stlport/ external/stlport/stlport/stl external/stlport/stlport/using/h/  bionic external/libnl/include/

LOCAL_SRC_FILES += \
	src/parameters/parameters.cpp \
	src/parameters/persistent.cpp \
	src/parameters/learn.cpp \
	src/process/powerconsumer.cpp \
	src/process/work.cpp \
	src/process/process.cpp \
	src/process/timer.cpp \
	src/process/device.cpp \
	src/process/interrupt.cpp \
	src/process/do_process.cpp \
	src/cpu/intel_cpus.cpp \
	src/cpu/cpu.cpp \
	src/cpu/cpu_linux.cpp \
	src/cpu/cpudevice.cpp \
	src/cpu/cpu_core.cpp \
	src/cpu/cpu_package.cpp \
	src/cpu/abstract_cpu.cpp \
	src/cpu/cpu_rapl_device.cpp \
	src/cpu/dram_rapl_device.cpp \
	src/cpu/rapl/rapl_interface.cpp \
	src/measurement/measurement.cpp \
	src/measurement/acpi.cpp \
	src/measurement/extech.cpp \
	src/measurement/sysfs.cpp \
	src/display.cpp \
	src/report.cpp \
	src/main.cpp \
	src/tuning/tuning.cpp \
	src/tuning/usb.cpp \
	src/tuning/bluetooth.cpp \
	src/tuning/ethernet.cpp \
	src/tuning/runtime.cpp \
	src/tuning/iw.c \
	src/tuning/iw.h \
	src/tuning/tunable.cpp \
	src/tuning/sysfs.cpp \
	src/tuning/cpufreq.cpp \
	src/tuning/wifi.cpp \
	src/perf/perf_bundle.cpp \
	src/perf/perf.cpp \
	src/devices/thinkpad-fan.cpp \
	src/devices/alsa.cpp \
	src/devices/runtime_pm.cpp \
	src/devices/usb.cpp \
	src/devices/ahci.cpp \
	src/devices/rfkill.cpp \
	src/devices/thinkpad-light.cpp \
	src/devices/i915-gpu.cpp \
	src/devices/backlight.cpp \
	src/devices/network.cpp \
	src/devices/device.cpp \
	src/devices/gpu_rapl_device.cpp \
	src/devlist.cpp \
	src/calibrate/calibrate.cpp \
	src/lib.cpp \
	traceevent/event-parse.c \
	traceevent/event-parse.h \
	traceevent/event-utils.h \
	traceevent/parse-filter.c \
	traceevent/parse-utils.c \
	traceevent/trace-seq.c

include $(BUILD_EXECUTABLE)
