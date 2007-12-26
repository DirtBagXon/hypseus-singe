bin_PROGRAMS = mpeg2dec extract_mpeg2
mpeg2dec_SOURCES = mpeg2dec.c getopt.c gettimeofday.c
mpeg2dec_LDADD = $(top_builddir)/libmpeg2/libmpeg2.la \
		 $(top_builddir)/libvo/libvo.a @LIBVO_LIBS@
extract_mpeg2_SOURCES = extract_mpeg2.c getopt.c

man_MANS = mpeg2dec.1 extract_mpeg2.1

EXTRA_DIST = getopt.h gettimeofday.h $(man_MANS)
