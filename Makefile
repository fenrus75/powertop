all: powertop

CFLAGS += -Wall -O2 -g
CPPFLAGS += -Wall -O2 -g
OBJS := main.o cpu.o cpu_linux.o cpu_core.o cpu_package.o intel_cpus.o
HEADERS := cpu.h


clean:
	rm -f *.o *~ powertop DEADJOE core.*
	
powertop: $(OBJS) $(HEADERS)
	g++ $(OBJS) -o powertop
