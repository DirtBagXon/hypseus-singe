# sub Makefile

%.d : %.cpp
	$(GEN_MSG)/$@
	$(SILENT)set -e; $(CXX) -MM $(CFLAGS) $< \
		| sed 's^\($*\)\.o[ :]*^\1.o $@ : ^g' > $@; \
		[ -s $@ ] || rm -f $@

OBJS = cpu.o mamewrap.o cpu-debug.o m80.o mc6809.o 6809infc.o nes6502.o \
	nes_6502.o cop.o copintf.o

.SUFFIXES:	.cpp

all:	${OBJS}

-include $(OBJS:.o=.d)

.cpp.o:
	$(CC_MSG)/$@
	$(SILENT)${CXX} ${CFLAGS} -c $< -o $@

clean:
	$(SILENT)rm -f ${OBJS} *.d
