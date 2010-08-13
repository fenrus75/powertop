all: powertop

CFLAGS += -Wall -O2 -g
CPPFLAGS += -Wall -O2 -g
CXXFLAGS += -Wall -O2 -g
OBJS := lib.o main.o cpu/cpu.o cpu/abstract_cpu.o cpu/cpu_linux.o cpu/cpu_core.o cpu/cpu_package.o cpu/intel_cpus.o 
OBJS += perf/perf.o perf/perf_bundle.o
OBJS += process/process.o

HEADERS := cpu/cpu.h


clean:
	rm -f *.o *~ powertop DEADJOE core.* */*.o */*~
	
powertop: $(OBJS) $(HEADERS)
	g++ $(OBJS) -o powertop
