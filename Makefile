all: powertop

CFLAGS += -Wall -O1 -g
CPPFLAGS += -Wall -O1 -g
CXXFLAGS += -Wall -O1 -g
OBJS := lib.o main.o 
OBJS += cpu/cpu.o cpu/abstract_cpu.o cpu/cpu_linux.o cpu/cpu_core.o cpu/cpu_package.o cpu/intel_cpus.o 
OBJS += perf/perf.o perf/perf_bundle.o
OBJS += process/process.o process/do_process.o process/interrupt.o process/timer.o process/work.o
OBJS += devices/device.o devices/backlight.o
OBJS += measurement/measurement.o measurement/acpi.o
OBJS += parameters/parameters.o

HEADERS := cpu/cpu.h


clean:
	rm -f *.o *~ powertop DEADJOE core.* */*.o */*~
	
powertop: $(OBJS) $(HEADERS)
	g++ $(OBJS) -o powertop
