#!/bin/sh
# Uso: ./rodar_testes.sh  (dentro da pasta testes, com ../ufabc-zipz compilado)
# Comprime, descomprime, confere com cmp e compara com o gzip.
ZIPZ=../ufabc-zipz
printf "%-12s %10s %10s %7s %9s | %10s %7s | %s\n" arquivo original zipz R tempo gzip R volta
for f in baixa.bin media.txt alta.txt; do
  rm -f "$f.zipz" "rest_$f"
  so=$(wc -c < "$f")
  saida=$(printf '1\n%s\n3\n' "$f" | $ZIPZ)
  sc=$(wc -c < "$f.zipz")
  t=$(echo "$saida" | sed -n 's/.*Tempo de compressao: \([0-9.]*\) s.*/\1/p')
  printf '2\n%s\nrest_%s\n3\n' "$f.zipz" "$f" | $ZIPZ > /dev/null
  if cmp -s "$f" "rest_$f"; then ok=OK; else ok=DIFERENTE; fi
  gzip -c "$f" > "$f.gz"
  sg=$(wc -c < "$f.gz")
  rz=$(awk -v a=$sc -v b=$so 'BEGIN{printf "%.3f", a/b}')
  rg=$(awk -v a=$sg -v b=$so 'BEGIN{printf "%.3f", a/b}')
  printf "%-12s %10s %10s %7s %8ss | %10s %7s | %s\n" "$f" "$so" "$sc" "$rz" "$t" "$sg" "$rg" "$ok"
done
