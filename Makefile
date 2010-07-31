all: powertop

CFLAGS += -Wall -O2 -g
CPPFLAGS += -Wall -O2 -g
OBJS := main.o cpu.o cpu_linux.o
HEADERS := cpu.h


clean:
	rm -f *.o *~ powertop DEADJOE
	
powertop: $(OBJS) $(HEADERS)
	g++ $(OBJS) -o powertop
