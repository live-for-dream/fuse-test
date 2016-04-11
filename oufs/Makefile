CC := gcc
CFLAGS := -g -Wall -D_FILE_OFFSET_BITS=64
#OBJS1 := $(patsubst %.c, %.o, $(wildcard *.c))
obj1 = oufs.o
obj2 = xmp.o
LIBS := -lfuse
 
TARGET1 := oufs
TARGET2 := xmp
 
.PHONY: all clean
 
all: $(TARGET1) $(TARGET2) 
 
$(TARGET1): $(obj1)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
$(TARGET2): $(obj2)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
.c.o:
	$(CC) $(CFLAGS) -c $<
 
clean:
	rm -f $(TARGET1) $(TARGET2) *.o