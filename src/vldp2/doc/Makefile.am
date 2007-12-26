noinst_PROGRAMS = sample1 sample2
sample1_SOURCES = sample1.c
sample1_LDADD = $(top_builddir)/libmpeg2/libmpeg2.la
sample2_SOURCES = sample2.c
sample2_LDADD = $(top_builddir)/libmpeg2/libmpeg2.la \
		$(top_builddir)/libvo/libvo.a @LIBVO_LIBS@

EXTRA_DIST = libmpeg2.txt libvo.txt
