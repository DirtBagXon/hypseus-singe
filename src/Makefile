# Makefile for DAPHNE
# Written by Matt Ownby

# You need to symlink the set of variables that correspond to that platform
# that you are using.  For example, if you are compiling for linux, you would
# "ln -s Makefile.vars.linux Makefile.vars"
# or make VARS=Makefile.vars.linux
ifeq ("$(wildcard Makefile.vars)","")
  ifneq ($(origin VARS),undefined)
    include ${VARS}
  else
    $(error Makefile.vars not found. Read documentation)
  endif
else
  include Makefile.vars
endif

ifndef VERBOSE
 export SILENT=@
endif
export CC_MSG = @echo Compiling $@
export AR_MSG = @echo Archiving $@
export AS_MSG = @echo Assembling $@
export EXE_MSG = @echo Creating executable: $@
export GEN_MSG = @echo Generating $@

ifneq ($(origin TOOLCHAIN),undefined)
export CC := $(TOOLCHAIN)-$(CC)
export CXX := $(TOOLCHAIN)-$(CXX)
export AR := $(TOOLCHAIN)-$(AR)
export AS := $(TOOLCHAIN)-$(AS)
endif
 
PKG_CONFIG=pkg-config
SDL_CONFIG=sdl2-config
STRIP=strip
WINDRES=windres

ifneq ($(origin CROSS),undefined)
export STRIP := $(TOOLCHAIN)-$(STRIP)
export PKG_CONFIG := $(TOOLCHAIN)-$(PKG_CONFIG)
export SDL_CONFIG := $(TOOLCHAIN)-$(SDL_CONFIG)
export WINDRES := $(TOOLCHAIN)-$(WINDRES)
endif

ifeq ($(USE_MMX),1)
PFLAGS += -DUSE_MMX
endif

# name of the executable and dynamic libraries
EXE_BASE = daphne
ifeq ($(OS),Windows_NT)
  EXE_EXT = .exe
endif
EXE = ../$(EXE_BASE)$(EXE_EXT)

# ...  by RDG2010
ifeq ($(BUILD_SINGE),1)
LIBS += $(shell ${PKG_CONFIG} --libs lua SDL2_mixer SDL2_image SDL2_ttf)
PFLAGS += -DBUILD_SINGE $(shell ${PKG_CONFIG} --cflags SDL2_mixer SDL2_image SDL2_ttf)
OBJS += game/singe/*.o
endif

VLDP_OBJS = vldp/*.o
PFLAGS += $(shell ${PKG_CONFIG} --cflags libmpeg2 libmpeg2convert)
LIBS += $(shell ${PKG_CONFIG} --libs libmpeg2 libmpeg2convert)

# Platform specific cflags defined in the Makefile.vars file
export CFLAGS = $(shell ${SDL_CONFIG} --cflags) ${PFLAGS} -Werror
LIBS += $(shell ${SDL_CONFIG} --libs)

OBJS += ldp-out/*.o cpu/*.o game/*.o io/*.o timer/*.o ldp-in/*.o video/*.o \
	sound/*.o daphne.o cpu/x86/*.o scoreboard/*.o ${VLDP_OBJS}

LOCAL_OBJS = daphne.o

.SUFFIXES:	.cpp

$(EXE):	${LOCAL_OBJS} subdirs
	$(EXE_MSG)
	$(SILENT)${CXX} ${DFLAGS} ${OBJS} -o ${EXE} ${LIBS}

SUBDIRS += ldp-out cpu cpu/x86 game io timer ldp-in video sound scoreboard vldp

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
ifndef VERBOSE
	$(SILENT)$(MAKE) -s -C $@
else
	$(SILENT)$(MAKE) -C $@
endif

-include $(LOCAL_OBJS:.o=.d)

.cpp.o:
	$(CC_MSG)
	$(SILENT)$(CXX) ${CFLAGS} -c $< -o $@

clean_deps:
	$(SILENT)find . -name "*.d" -exec rm {} \;

clean:	clean_deps
	$(SILENT)rm -f ${OBJS}
	$(SILENT)find . -name "*.o" -exec rm {} \;
	$(SILENT)rm -f ${EXE}

testvldp:	testvldp.c
	${CC} ${CFLAGS} -DSHOW_FRAMES -DUSE_OVERLAY testvldp.c ${VLDP_OBJS} ${LIBS} -o ../testvldp$(EXE_EXT)

%.d : %.cpp
	$(GEN_MSG)
	$(SILENT)set -e; $(CXX) -MM $(CFLAGS) $< \
                | sed 's^\($*\)\.o[ :]*^\1.o $@ : ^g' > $@; \
                [ -s $@ ] || rm -f $@

