TARGET = emu
LIBS = -lm -lsdl2
CC = gcc
CFLAGS = -g -Wall

.PHONY: default all clean

default: $(TARGET)
all: default

SRC_FILES = $(wildcard *.c)
SRC_FILES := $(filter-out test.c, $(wildcard *.c))
OBJECTS = $(patsubst %.c, %.o, $(SRC_FILES))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)