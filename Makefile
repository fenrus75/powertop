all: powertop 

CFLAGS += -Wall -O2 -g -fno-omit-frame-pointer
CPPFLAGS += -Wall -O2 -g -fno-omit-frame-pointer
CXXFLAGS += -Wall -O2 -g -fno-omit-frame-pointer
OBJS := lib.o main.o display.o html.o
OBJS += cpu/cpu.o cpu/abstract_cpu.o cpu/cpu_linux.o cpu/cpu_core.o cpu/cpu_package.o cpu/intel_cpus.o  cpu/cpudevice.cpp
OBJS += perf/perf.o perf/perf_bundle.o
OBJS += process/process.o process/do_process.o process/interrupt.o process/timer.o process/work.o process/powerconsumer.o process/device.o
DEVS += devices/device.o devices/backlight.o devices/usb.o devices/ahci.o devices/alsa.o devices/rfkill.o devices/i915-gpu.o devices/thinkpad-fan.o devices/network.o
DEVS += devices/runtime_pm.o
DEVS += measurement/measurement.o measurement/acpi.o
OBJS += $(DEVS)
OBJS += parameters/parameters.o parameters/learn.o parameters/persistent.o
OBJS += calibrate/calibrate.o

OBJS += tuning/tuning.o tuning/tunable.o tuning/sysfs.o tuning/usb.o tuning/runtime.o

#
# ncurses-devel and pciutils-devel 
#

LIBS += -lpthread -lncursesw -lpci -lz -lresolv

HEADERS := cpu/cpu.h


clean:
	rm -f *.o *~ powertop DEADJOE core.* */*.o */*~
	
powertop: $(OBJS) $(HEADERS)
	g++ $(OBJS) $(LIBS) -o powertop



%.o: %.cpp lib.h Makefile
	@echo "  CC  $<"
	@[ -x /usr/bin/cppcheck ] && /usr/bin/cppcheck -q $< || :
	@$(CC) $(CFLAGS) -c -o $@ $<
