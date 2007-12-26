# sub Makefile

%.d : %.cpp
	set -e; $(CXX) -MM $(CFLAGS) $< \
		| sed 's^\($*\)\.o[ :]*^\1.o $@ : ^g' > $@; \
		[ -s $@ ] || rm -f $@

OBJS = sound.o ssi263.o tqsynth.o sn_intf.o tms9919-sdl.o tms9919.o \
	pc_beeper.o gisound.o dac.o tonegen.o samples.o mix.o

.SUFFIXES:	.cpp

all:	${OBJS} mix_mmx-asm

include $(OBJS:.o=.d)

.cpp.o:
	${CXX} ${CFLAGS} -c $< -o $@

# MMX audio mixing function (conditional upon whether USE_MMX is defined)
mix_mmx-asm:  mix_mmx-gas.s
ifeq ($(USE_MMX),1)
	as mix_mmx-gas.s -o mix_mmx-gas.o
endif

clean:
	rm -f ${OBJS} *.d
