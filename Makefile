CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic
TARGET = rls_analyze
OBJS = main.o tracks.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c tracks.h
	$(CC) $(CFLAGS) -c main.c

tracks.o: tracks.c tracks.h
	$(CC) $(CFLAGS) -c tracks.c

clean:
	rm -f $(TARGET) $(OBJS)
