# sub Makefile

%.d : %.cpp
	set -e; $(CXX) -MM $(CFLAGS) $< \
		| sed 's^\($*\)\.o[ :]*^\1.o $@ : ^g' > $@; \
		[ -s $@ ] || rm -f $@

OBJS = input.o serial.o conout.o cmdline.o conin.o parallel.o error.o \
	network.o sram.o fileparse.o unzip.o mpo_fileio.o numstr.o homedir.o \
	logger.o logger_console.o logger_factory.o

.SUFFIXES:	.cpp

all:	${OBJS}

include $(OBJS:.o=.d)

.cpp.o:
	${CXX} ${CFLAGS} -c $< -o $@

clean:
	rm -f ${OBJS} *.d
