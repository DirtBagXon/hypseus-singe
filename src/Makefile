# Makefile for DAPHNE
# Written by Matt Ownby

# You need to symlink the set of variables that correspond to that platform
# that you are using.  For example, if you are compiling for linux, you would
# do "ln -s Makefile.vars.linux Makefile.vars"
ifeq ("$(wildcard Makefile.vars)","")
  $(error Makefile.vars not found. Read documentation)
endif

include Makefile.vars

ifndef VERBOSE
 export SILENT=@
endif
export CC_MSG = @echo Compiling $@
export AR_MSG = @echo Archiving $@
export EXE_MSG = @echo Creating executable: $@
export GEN_MSG = @echo Generating $@

ifneq ($(origin TOOLCHAIN),undefined)
export CC := $(TOOLCHAIN)-$(CC)
export CXX := $(TOOLCHAIN)-$(CXX)
export AR := $(TOOLCHAIN)-$(AR)
endif
 
PKG_CONFIG=pkg-config
STRIP=strip
WINDRES=windres

ifneq ($(origin CROSS),undefined)
export STRIP := $(TOOLCHAIN)-$(STRIP)
export PKG_CONFIG := $(TOOLCHAIN)-$(PKG_CONFIG)
export WINDRES := $(TOOLCHAIN)-$(WINDRES)
endif

# send these to all the sub-Makefiles

# name of the executable and dynamic libraries
EXE_BASE = daphne.bin
ifeq ($(OS),Windows_NT)
  EXE_EXT = .exe
endif
EXE = ../$(EXE_BASE)$(EXE_EXT)

# ...  by RDG2010
ifeq ($(BUILD_SINGE),1)
LIBS += -lSDL_image -lSDL_ttf $(shell pkg-config --libs lua)
PFLAGS += -DBUILD_SINGE
OBJS += game/singe/*.o
endif

VLDP_OBJS = vldp/*.o
PFLAGS += $(shell pkg-config --cflags libmpeg2 libmpeg2convert)
LIBS += $(shell pkg-config --libs libmpeg2 libmpeg2convert)

# Platform specific cflags defined in the Makefile.vars file
export CFLAGS = ${PFLAGS} -Wall

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
	$(SILENT)find . -name "*.o" -exec rm {} \;
	$(SILENT)rm -f ${EXE}
	$(SILENT)rm -f ${LIB_VLDP}
	$(SILENT)rm -f ${LIB_SINGE}

testvldp:	testvldp.c
	${CC} ${CFLAGS} -DSHOW_FRAMES -DUSE_OVERLAY testvldp.c ${VLDP_OBJS} ${LIBS} -o ../testvldp

%.d : %.cpp
	$(GEN_MSG)
	$(SILENT)set -e; $(CXX) -MM $(CFLAGS) $< \
                | sed 's^\($*\)\.o[ :]*^\1.o $@ : ^g' > $@; \
                [ -s $@ ] || rm -f $@

