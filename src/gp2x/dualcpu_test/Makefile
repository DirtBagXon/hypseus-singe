
CROSS_COMPILE = /opt/open2x/gcc*/bin/arm-open2x-linux-
LDFLAGS = -static

CXX = $(CROSS_COMPILE)gcc

CXXFLAGS = -I/opt/open2x/gcc*/include -Wall -Werror
LIBS = -L/opt/open2x/gcc*/lib -lpthread

TEST_TARGET = test.gpe
TEST_OBJS = main1.o

all : $(TEST_TARGET)

$(TEST_TARGET) : $(TEST_OBJS)
	$(CXX) $(LDFLAGS) -o $(TEST_TARGET) $(TEST_OBJS) $(LIBS)

main1.o: main1.c
	$(CXX) $(CXXFLAGS) -O2 -c main1.c
