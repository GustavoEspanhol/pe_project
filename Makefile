CC = gcc
CFLAGS = -std=c89 -Wall -Wextra -pedantic
TARGET = ufabc-zipz
OBJECTS = main.o compressor.o huffman_tree.o min_heap.o dictionary.o bitio.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

main.o: main.c compressor.h
	$(CC) $(CFLAGS) -c main.c

compressor.o: compressor.c compressor.h huffman_tree.h dictionary.h bitio.h
	$(CC) $(CFLAGS) -c compressor.c

huffman_tree.o: huffman_tree.c huffman_tree.h min_heap.h
	$(CC) $(CFLAGS) -c huffman_tree.c

min_heap.o: min_heap.c min_heap.h huffman_tree.h
	$(CC) $(CFLAGS) -c min_heap.c

dictionary.o: dictionary.c dictionary.h huffman_tree.h
	$(CC) $(CFLAGS) -c dictionary.c

bitio.o: bitio.c bitio.h
	$(CC) $(CFLAGS) -c bitio.c

clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: all clean