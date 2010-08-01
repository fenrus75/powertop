all: powertop

CFLAGS += -Wall -O2 -g
CPPFLAGS += -Wall -O2 -g
OBJS := lib.o main.o cpu.o abstract_cpu.o cpu_linux.o cpu_core.o cpu_package.o intel_cpus.o 
HEADERS := cpu.h


clean:
	rm -f *.o *~ powertop DEADJOE core.*
	
powertop: $(OBJS) $(HEADERS)
	g++ $(OBJS) -o powertop
