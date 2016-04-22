# sub Makefile

%.d : %.cpp
	$(GEN_MSG)/$@
	$(SILENT)set -e; $(CXX) -MM $(CFLAGS) $< \
		| sed 's^\($*\)\.o[ :]*^\1.o $@ : ^g' > $@; \
		[ -s $@ ] || rm -f $@

OBJS = sony.o pioneer.o ld-v6000.o hitachi.o philips.o ldp.o framemod.o \
	ldp-combo.o ldp-vldp.o ldp-vldp-audio.o

.SUFFIXES:	.cpp

all:	$(OBJS)

-include $(OBJS:.o=.d)

.cpp.o:
	$(CC_MSG)/$@
	$(SILENT)$(CXX) $(CFLAGS) -c $< -o $@

clean:
	$(SILENT)rm -f $(OBJS) *.d
