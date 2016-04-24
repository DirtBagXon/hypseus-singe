%.d : %.cpp
	$(GEN_MSG)/$@
	$(SILENT)set -e; $(CXX) -MM $(CFLAGS) $< \
		| sed 's^\($*\)\.o[ :]*^\1.o $@ : ^g' > $@; \
		[ -s $@ ] || rm -f $@

%.d : %.c
	$(GEN_MSG)/$@
	$(SILENT)set -e; $(CC) -MM $(CFLAGS) $< \
		| sed 's^\($*\)\.o[ :]*^\1.o $@ : ^g' > $@; \
		[ -s $@ ] || rm -f $@

OBJS = vldp.o vldp_internal.o mpegscan.o

all:	${OBJS}

-include $(OBJS:.o=.d)

.cpp.o:
	$(CC_MSG)/$@
	$(SILENT)${CXX} ${CFLAGS} -c $< -o $@

.c.o:
	$(CC_MSG)/$@
	$(SILENT)${CC} ${CFLAGS} -c $< -o $@

clean:
	$(SILENT)rm -f ${OBJS} *.d
