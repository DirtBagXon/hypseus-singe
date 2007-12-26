# sub Makefile

%.d : %.cpp
	set -e; $(CXX) -MM $(CFLAGS) $< \
		| sed 's^\($*\)\.o[ :]*^\1.o $@ : ^g' > $@; \
		[ -s $@ ] || rm -f $@

OBJS = sony.o pioneer.o ld-v6000.o hitachi.o philips.o ldp.o framemod.o \
	ldp-combo.o ldp-vldp.o ldp-vldp-audio.o ldp-vldp-gl.o ldp-vldp-gp2x.o

.SUFFIXES:	.cpp

all:	${OBJS}

include $(OBJS:.o=.d)

.cpp.o:
	${CXX} ${CFLAGS} -c $< -o $@

clean:
	rm -f ${OBJS} *.d
