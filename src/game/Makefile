# sub Makefile

%.d : %.cpp
	$(GEN_MSG)/$@
	$(SILENT)set -e; $(CXX) -MM $(CFLAGS) $< \
		| sed 's^\($*\)\.o[ :]*^\1.o $@ : ^g' > $@; \
		[ -s $@ ] || rm -f $@

LOCAL_OBJS = game.o boardinfo.o lair.o superd.o thayers.o astron.o firefox.o \
	cliff.o speedtest.o seektest.o cputest.o ffr.o esh.o laireuro.o \
	badlands.o starrider.o bega.o multicputest.o cobraconv.o gpworld.o \
        interstellar.o benchmark.o lair2.o mach3.o lgp.o timetrav.o \
	releasetest.o singe.o test_sb.o

SUBDIRS = 

ifeq ($(BUILD_SINGE),1)
SUBDIRS += singe
endif

all:	${LOCAL_OBJS} subdirs

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(SILENT)$(MAKE) -s -C $@

-include $(LOCAL_OBJS:.o=.d)

.cpp.o:
	$(CC_MSG)/$@
	$(SILENT)${CXX} ${CFLAGS} -c $< -o $@

clean:
	$(SILENT)rm -f ${LOCAL_OBJS} *.d
