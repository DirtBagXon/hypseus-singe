AM_CFLAGS = @LIBMPEG2_CFLAGS@

lib_LTLIBRARIES = libmpeg2.la
libmpeg2_la_SOURCES = alloc.c cpu_accel.c header.c decode.c cpu_state.c \
		      slice.c motion_comp_mmx.c idct_mmx.c \
		      motion_comp_altivec.c idct_altivec.c \
		      motion_comp_alpha.c idct_alpha.c \
		      motion_comp_mlib.c idct_mlib.c \
		      motion_comp.c idct.c
libmpeg2_la_LIBADD = @LIBMPEG2_LIBS@
libmpeg2_la_LDFLAGS = -no-undefined

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmpeg2.pc

EXTRA_DIST = configure.incl vlc.h mpeg2_internal.h
