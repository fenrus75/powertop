all: powertop  po/powertop.pot

VERSION := 1.98

CFLAGS += -Wall -O2 -g -fno-omit-frame-pointer -fstack-protector -Wshadow -Wformat -D_FORTIFY_SOURCE=2
CPPFLAGS += -Wall -O2 -g -fno-omit-frame-pointer
CXXFLAGS += -Wall -O2 -g -fno-omit-frame-pointer -fstack-protector -Wshadow -Wformat -D_FORTIFY_SOURCE=2
PKG_CONFIG ?= pkg-config

OBJS := lib.o main.o display.o html.o devlist.o
OBJS += cpu/cpu.o cpu/abstract_cpu.o cpu/cpu_linux.o cpu/cpu_core.o cpu/cpu_package.o cpu/intel_cpus.o  cpu/cpudevice.cpp
OBJS += perf/perf.o perf/perf_bundle.o
OBJS += process/process.o process/do_process.o process/interrupt.o process/timer.o process/work.o process/powerconsumer.o process/device.o
DEVS += devices/device.o devices/backlight.o devices/usb.o devices/ahci.o devices/alsa.o devices/rfkill.o devices/i915-gpu.o devices/thinkpad-fan.o devices/network.o devices/thinkpad-light.o
DEVS += devices/runtime_pm.o
DEVS += measurement/measurement.o measurement/acpi.o measurement/extech.o measurement/power_supply.o
OBJS += $(DEVS)
OBJS += parameters/parameters.o parameters/learn.o parameters/persistent.o
OBJS += calibrate/calibrate.o

OBJS += tuning/tuning.o tuning/tunable.o tuning/sysfs.o tuning/usb.o tuning/runtime.o tuning/bluetooth.o
OBJS += tuning/cpufreq.o tuning/ethernet.o tuning/iw.o tuning/wifi.o


NL1FOUND := $(shell $(PKG_CONFIG) --atleast-version=1 libnl-1 && echo Y)
NL2FOUND := $(shell $(PKG_CONFIG) --atleast-version=2 libnl-2.0 && echo Y)
NL3FOUND := $(shell $(PKG_CONFIG) --atleast-version=3 libnl-3.0 && echo Y)

ifeq ($(NL1FOUND),Y)
NLLIBNAME = libnl-1
endif

ifeq ($(NL2FOUND),Y)
CFLAGS += -DCONFIG_LIBNL20
LIBS += -lnl-genl
NLLIBNAME = libnl-2.0
endif

ifeq ($(NL3FOUND),Y)
CFLAGS += -DCONFIG_LIBNL20
LIBS += -lnl-genl
NLLIBNAME = libnl-3.0
endif

ifeq ($(NLLIBNAME),)
$(error Cannot find development files for any supported version of libnl)
endif

LIBS += $(shell $(PKG_CONFIG) --libs $(NLLIBNAME))
CFLAGS += $(shell $(PKG_CONFIG) --cflags $(NLLIBNAME))



#
# ncurses-devel and pciutils-devel 
#

LIBS += -lpthread -lncursesw -lpci -lz -lresolv

HEADERS := cpu/cpu.h


PREFIX     ?= /usr
BINDIR      = $(PREFIX)/bin
LOCALESDIR  = $(PREFIX)/share/locale
MANDIR      = $(PREFIX)/share/man/man8


clean:
	rm -f *.o *~ powertop DEADJOE core.* */*.o */*~ csstoh css.h
	
powertop: $(OBJS) $(HEADERS)
	$(CXX) $(OBJS) $(LIBS) -o powertop
	@(cd po/ && $(MAKE))
	
install: powertop
	mkdir -p ${DESTDIR}${BINDIR}
	cp powertop ${DESTDIR}${BINDIR}
	mkdir -p ${DESTDIR}/var/cache/powertop
	@(cd po/ && env LOCALESDIR=$(LOCALESDIR) DESTDIR=$(DESTDIR) $(MAKE) $@)
	

csstoh: csstoh.c
	gcc -o csstoh csstoh.c

css.h: csstoh powertop.css
	./csstoh powertop.css css.h


%.o: %.cpp lib.h css.h Makefile
	@echo "  CC  $<"
	@[ -x /usr/bin/cppcheck ] && /usr/bin/cppcheck -q $< || :
	@$(CC) $(CFLAGS) -c -o $@ $<


uptrans:
	@(cd po/ && env LG=$(LG) $(MAKE) $@)


po/powertop.pot: *.cpp */*.cpp *.h */*.h
	xgettext -C -s -k_ -o po/powertop.pot *.cpp */*.cpp *.h */*.h


dist:
	git tag v$(VERSION)
	git archive --format=tar --prefix="powertop-$(VERSION)/" v$(VERSION) | \
		gzip > powertop-$(VERSION).tar.gz
