all: powertop

CFLAGS += -Wall -O0 -g
CPPFLAGS += -Wall -O0 -g
CXXFLAGS += -Wall -O0 -g
OBJS := lib.o main.o 
OBJS += cpu/cpu.o cpu/abstract_cpu.o cpu/cpu_linux.o cpu/cpu_core.o cpu/cpu_package.o cpu/intel_cpus.o  cpu/cpudevice.cpp
OBJS += perf/perf.o perf/perf_bundle.o
OBJS += process/process.o process/do_process.o process/interrupt.o process/timer.o process/work.o process/powerconsumer.o process/device.o
OBJS += devices/device.o devices/backlight.o devices/usb.o devices/ahci.o devices/alsa.o devices/rfkill.o
OBJS += measurement/measurement.o measurement/acpi.o
OBJS += parameters/parameters.o parameters/learn.o parameters/persistent.o
OBJS += calibrate/calibrate.o

LIBS += -lpthread

HEADERS := cpu/cpu.h


clean:
	rm -f *.o *~ powertop DEADJOE core.* */*.o */*~
	
powertop: $(OBJS) $(HEADERS)
	g++ $(OBJS) $(LIBS) -o powertop
