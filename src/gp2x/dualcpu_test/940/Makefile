
CROSS_COMPILE = /opt/open2x/gcc*/bin/arm-open2x-linux-
LDFLAGS = -static

CXX = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
STRIP = $(CROSS_COMPILE)strip

CXXFLAGS = -I/opt/open2x/gcc*/include -Wall -Werror
LIBS = -L/opt/open2x/gcc*/lib

CODE940_TARGET = code940.gpe
CODE940_OBJS = main.o 940.o

all : $(CODE940_TARGET)

$(CODE940_TARGET) : $(CODE940_OBJS)
	$(LD) -e code940 -Ttext 0x0 $(CODE940_OBJS)  -o $(CODE940_TARGET)
	$(CROSS_COMPILE)objcopy -O binary code940.gpe code940.bin
	ls -l code940.bin
	cp code940.bin ..

main.o: main.c
	$(CXX) $(CXXFLAGS) -O0 -c main.c

940.o: 940.c
	$(CXX) $(CXXFLAGS) -O2 -c 940.c
