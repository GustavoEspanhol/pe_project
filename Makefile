CC = gcc
CFLAGS = -std=c89 -Wall -Wextra -pedantic
LDLIBS = -lm
TARGET = ufabc-zipz
OBJECTS = main.o compressor.o decoder.o search.o analysis.o huffman_tree.o \
          min_heap.o dictionary.o bitio.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDLIBS)

main.o: main.c analysis.h compressor.h format.h search.h
	$(CC) $(CFLAGS) -c main.c

compressor.o: compressor.c compressor.h decoder.h format.h huffman_tree.h dictionary.h bitio.h
	$(CC) $(CFLAGS) -c compressor.c

decoder.o: decoder.c decoder.h format.h huffman_tree.h bitio.h
	$(CC) $(CFLAGS) -c decoder.c

search.o: search.c search.h decoder.h
	$(CC) $(CFLAGS) -c search.c

analysis.o: analysis.c analysis.h format.h huffman_tree.h
	$(CC) $(CFLAGS) -c analysis.c

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
