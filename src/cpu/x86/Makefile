# sub Makefile

%.d : %.cpp
	$(GEN_MSG)/$@
	$(SILENT)set -e; $(CXX) -MM $(CFLAGS) $< \
		| sed 's^\($*\)\.o[ :]*^\1.o $@ : ^g' > $@; \
		[ -s $@ ] || rm -f $@

OBJS = i86.o i86dasm.o

.SUFFIXES:	.cpp

all:	${OBJS}

-include $(OBJS:.o=.d)

.cpp.o:
	$(CC_MSG)
	$(SILENT)${CXX} ${CFLAGS} -c $< -o $@

clean:
	$(SILENT)rm -f ${OBJS} *.d
