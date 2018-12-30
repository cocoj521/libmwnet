CC 	= g++
CFLAGS  += -DLINUX -DSQL_WCHART_CONVERT
LIB 	+= 
INCLUDE += 

#version type(debug or release)
ifeq ($(VERTYPE),DEBUG)
CFLAGS += -Wall -g -DDEBUG
TARGET += ./testhttpparser
endif
ifeq ($(VERTYPE),RELEASE)
CFLAGS +=
TARGET += ./testhttpparser
endif

ifeq ($(GCCTYPE),OLD)
CC = g++343
endif
ifeq ($(GCCTYPE),NEW)
CC = g++
endif

#remote objects
ROBJS = 

#local objects
LOBJS = http_parser.o httpparser.o test.o

#dependent objects
OBJS = $(ROBJS) $(LOBJS)

$(TARGET): $(OBJS)
	$(CC) $(INCLUDE) $(CFLAGS) -o $@ $(OBJS) -lpthread 
.cpp.o:
	$(CC) $(INCLUDE) $(CFLAGS) -c $<
clean:
	rm $(LOBJS)
	rm $(TARGET)
	rm ./*.o
