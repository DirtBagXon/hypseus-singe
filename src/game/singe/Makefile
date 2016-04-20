# sub Makefile

%.d : %.cpp
	$(GEN_MSG)/$@
	$(SILENT)set -e; $(CXX) -MM $(CFLAGS) $< \
		| sed 's^\($*\)\.o[ :]*^\1.o $@ : ^g' > $@; \
		[ -s $@ ] || rm -f $@

SUB_CFLAGS = $(shell ${PKG_CONFIG} --cflags lua)

OBJS = singeproxy.o

.SUFFIXES:	.cpp

all:	${OBJS}

-include $(OBJS:.o=.d)

.cpp.o:
	$(CC_MSG)/$@
	$(SILENT)${CXX} ${CFLAGS} ${SUB_CFLAGS} -c $< -o $@

clean:
	$(SILENT)rm -f ${OBJS} *.d
