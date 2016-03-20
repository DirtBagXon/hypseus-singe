# sub Makefile

%.d : %.cpp
	$(GEN_MSG)/$@
	$(SILENT)set -e; $(CXX) -MM $(CFLAGS) $< \
		| sed 's^\($*\)\.o[ :]*^\1.o $@ : ^g' > $@; \
		[ -s $@ ] || rm -f $@

OBJS = scoreboard_interface.o scoreboard_factory.o scoreboard_collection.o \
	hw_scoreboard.o img_scoreboard.o null_scoreboard.o \
	overlay_scoreboard.o

.SUFFIXES:	.cpp

all:	${OBJS}

-include $(OBJS:.o=.d)

.cpp.o:
	$(CC_MSG)/$@
	$(SILENT)${CXX} ${CFLAGS} -c $< -o $@

clean:
	$(SILENT)rm -f ${OBJS} *.d
