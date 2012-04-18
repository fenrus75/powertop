all: libparseevent.a

PEVENT_LIB_OBJS = parse-events.o trace-seq.o parse-filter.o parse-utils.o

libparseevent.a: $(PEVENT_LIB_OBJS)
	$(RM) $@;  $(AR) rcs $@ $^

clean:
	$(RM) $(PEVENT_LIB_OBJS) libparseevent.a