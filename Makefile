CC = gcc
CFLAGS = -std=c89 -Wall -Wextra -pedantic -O2

TARGET = ufabc_zipz
SOURCES = main.c compressor.c bitio.c dictionary.c huffman_tree.c min_heap.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
