# Makefile for DAPHNE
# Written by Matt Ownby

# You need to symlink the set of variables that correspond to that platform
# that you are using.  For example, if you are compiling for linux, you would
# do "ln -s Makefile.vars.linux Makefile.vars"
include Makefile.vars

# send these to all the sub-Makefiles

# name of the executable and dynamic libraries
EXE = ../daphne.bin
LIB_VLDP = ../libvldp2.so
LIB_SINGE = ../libsinge.so

VLDP_OBJS =
SINGE_OBJS =

# if we are statically linking VLDP (instead of dynamic)
# NOTE : these libs must be compiled separately beforehand (as if building a shared vldp)
ifeq ($(STATIC_VLDP),1)
VLDP_OBJS = vldp2/vldp/vldp.o vldp2/vldp/vldp_internal.o vldp2/vldp/mpegscan.o \
	vldp2/libmpeg2/cpu_accel.o vldp2/libmpeg2/alloc.o vldp2/libmpeg2/cpu_state.o vldp2/libmpeg2/decode.o \
	vldp2/libmpeg2/header.o vldp2/libmpeg2/motion_comp.o vldp2/libmpeg2/idct.o vldp2/libmpeg2/idct_mmx.o \
	vldp2/libmpeg2/motion_comp_mmx.o vldp2/libmpeg2/slice.o vldp2/libvo/video_out.o vldp2/libvo/video_out_null.o
DEFINE_STATIC_VLDP = -DSTATIC_VLDP
endif

# if doing the same for SINGE...  by RDG2010
ifeq ($(STATIC_SINGE),1)
SINGE_OBJS =  game/singe/lbaselib.o game/singe/ldblib.o \
	game/singe/ldump.o game/singe/lapi.o game/singe/lauxlib.o \
	game/singe/lcode.o game/singe/ldebug.o game/singe/ldo.o \
	game/singe/lfunc.o game/singe/linit.o game/singe/lgc.o \
	game/singe/liolib.o game/singe/llex.o game/singe/lmathlib.o \
	game/singe/lmem.o game/singe/loadlib.o game/singe/lobject.o \
	game/singe/lopcodes.o game/singe/loslib.o game/singe/lparser.o \
	game/singe/lstate.o game/singe/lstring.o game/singe/lstrlib.o game/singe/ltable.o \
	game/singe/ltablib.o game/singe/ltm.o game/singe/lundump.o \
	game/singe/lvm.o game/singe/lzio.o game/singe/lrandom.o game/singe/random.o \
	game/singe/singeproxy.o

DEFINE_STATIC_SINGE = -DSTATIC_SINGE
endif

# gp2x static linking is slightly different because the decoding
#  is done on the 940 cpu
ifeq ($(STATIC_VLDP_GP2X),1)
VLDP_OBJS = vldp2/vldp/vldp.o vldp2/vldp/vldp_internal.o vldp2/vldp/mpegscan.o \
	vldp2/libvo/video_out_null.o vldp2/940/interface_920.o
EXE = ../daphne2x
DEFINE_STATIC_VLDP = -DSTATIC_VLDP
endif


# Platform specific cflags defined in the Makefile.vars file
#export CFLAGS = ${PFLAGS} ${DEFINE_STATIC_VLDP} ${DEFINE_STATIC_SINGE} -Wall -Werror
export CFLAGS = ${PFLAGS} ${DEFINE_STATIC_VLDP} ${DEFINE_STATIC_SINGE} -Wall

OBJS = ldp-out/*.o cpu/*.o game/*.o io/*.o timer/*.o ldp-in/*.o video/*.o \
	sound/*.o daphne.o cpu/x86/*.o scoreboard/*.o ${SINGE_OBJS} ${VLDP_OBJS} 

LOCAL_OBJS = daphne.o

.SUFFIXES:	.cpp

all:	${LOCAL_OBJS} sub
	${CXX} ${DFLAGS} ${OBJS} -o ${EXE} ${LIBS}

sub:
	cd ldp-out && $(MAKE)
	cd cpu && $(MAKE)
	cd cpu/x86 && $(MAKE)
	cd game && $(MAKE)
	cd io && $(MAKE)
	cd timer && $(MAKE)
	cd ldp-in && $(MAKE)
	cd video && $(MAKE)
	cd sound && $(MAKE)
	cd scoreboard && $(MAKE)

include $(LOCAL_OBJS:.o=.d)

.cpp.o:
	${CXX} ${CFLAGS} -c $< -o $@

clean_deps:
	find . -name "*.d" -exec rm {} \;

clean:	clean_deps
	find . -name "*.o" -exec rm {} \;
	rm -f ${EXE}
	rm -f ${LIB_VLDP}
	rm -f ${LIB_SINGE}

testvldp:	testvldp.c
	${CC} ${CFLAGS} -DSHOW_FRAMES -DUSE_OVERLAY testvldp.c ${VLDP_OBJS} ${LIBS} -o ../testvldp

%.d : %.cpp
	set -e; $(CXX) -MM $(CFLAGS) $< \
                | sed 's^\($*\)\.o[ :]*^\1.o $@ : ^g' > $@; \
                [ -s $@ ] || rm -f $@

