all: powertop

OBJS := main.o cpu.o
HEADERS := cpu.h


clean:
	rm -f *.o *~ powertop DEADJOE
	
powertop: $(OBJS) $(HEADERS)
	g++ $(OBJS) -o powertop
