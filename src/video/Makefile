# sub Makefile

%.d : %.cpp
	$(GEN_MSG)/$@
	$(SILENT)set -e; $(CXX) -MM $(CFLAGS) $< \
		| sed 's^\($*\)\.o[ :]*^\1.o $@ : ^g' > $@; \
		[ -s $@ ] || rm -f $@

OBJS = video.o tms9128nl.o SDL_DrawText.o \
	led.o palette.o rgb2yuv.o blend.o

.SUFFIXES:	.cpp

all:	${OBJS} rgb2yuv-asm blend_mmx-asm

-include $(OBJS:.o=.d)

.cpp.o:
	$(CC_MSG)/$@
	$(SILENT)${CXX} ${CFLAGS} -c $< -o $@

# this assembly language code is MMX only, so we have a conditional here
# so that non-MMX cpu's don't choke here ...
rgb2yuv-asm:	rgb2yuv-gas.s
ifeq ($(USE_MMX),1)
	$(AS_MSG)/$@
	$(SILENT)${AS} rgb2yuv-gas.s -o rgb2yuv-gas.o
endif

blend_mmx-asm:	blend_mmx-gas.s
ifeq ($(USE_MMX),1)
	$(AS_MSG)/$@
	$(SILENT)${AS} blend_mmx-gas.s -o blend_mmx-gas.o
endif

clean:
	$(SILENT)rm -f ${OBJS} *.d
