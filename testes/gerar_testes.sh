#!/bin/sh
# Gera os arquivos de teste (1 MiB aprox.) nos tres cenarios de redundancia.
dd if=/dev/urandom of=baixa.bin bs=1048576 count=1 status=none
awk 'BEGIN { for (i=0; i<20000; i++) printf "registro %05d UFABC Huffman dados estruturados\n", i % 1000 }' > media.txt
awk 'BEGIN { for (i=0; i<30000; i++) print "UFABC Zipz comprime dados repetidos com Huffman" }' > alta.txt
ls -l baixa.bin media.txt alta.txt
