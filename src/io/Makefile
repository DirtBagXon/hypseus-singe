# sub Makefile

%.d : %.cpp
	$(GEN_MSG)/$@
	$(SILENT)set -e; $(CXX) -MM $(CFLAGS) $< \
		| sed 's^\($*\)\.o[ :]*^\1.o $@ : ^g' > $@; \
		[ -s $@ ] || rm -f $@

OBJS = input.o serial.o conout.o conin.o parallel.o error.o \
	network.o sram.o fileparse.o unzip.o mpo_fileio.o numstr.o homedir.o \
	logger.o logger_console.o logger_factory.o cmdline.o

.SUFFIXES:	.cpp

all:	${OBJS}

-include $(OBJS:.o=.d)

.cpp.o:
	$(CC_MSG)/$@
	$(SILENT)${CXX} ${CFLAGS} -c $< -o $@

clean:
	$(SILENT)rm -f ${OBJS} *.d
