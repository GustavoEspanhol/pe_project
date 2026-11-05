#!/bin/sh
# Uso: ./rodar_testes.sh  (dentro da pasta testes, com ../ufabc-zipz compilado)
# Para cada cenario: analise de redundancia, compressao, descompressao,
# conferencia com cmp e comparacao com o gzip.
# Tempos do Zipz vem do proprio programa; os do gzip sao medidos com date.
ZIPZ=../ufabc-zipz
[ -x "$ZIPZ" ] || { echo "Compile antes com make na pasta do projeto."; exit 1; }

agora() { date +%s.%N; }
diferenca() { awk -v a="$1" -v b="$2" 'BEGIN{printf "%.3f", b-a}'; }
razao() { awk -v a="$1" -v b="$2" 'BEGIN{printf "%.3f", a/b}'; }

printf "%-10s %8s %6s %6s %-6s | %8s %6s %6s %6s | %8s %6s %6s %6s | %s\n" \
  arquivo original H Rmin cen zipz R comp desc gzip R comp desc volta
for f in baixa.bin media.txt alta.txt; do
  [ -f "$f" ] || { echo "$f nao existe, rode ./gerar_testes.sh"; continue; }
  rm -f "$f.zipz" "rest_$f" "$f.gz" "gzrest_$f"
  so=$(wc -c < "$f")

  analise=$(printf '5\n%s\n3\n' "$f" | $ZIPZ)
  h=$(echo "$analise" | sed -n 's/^Entropia: \([0-9.]*\).*/\1/p')
  rmin=$(echo "$analise" | sed -n 's/^Razao minima teorica.*: \([0-9.]*\)/\1/p')
  cen=$(echo "$analise" | sed -n 's/^Cenario: \([A-Z]*\).*/\1/p')

  # o "s" responde a pergunta feita quando nao compensa comprimir
  saida=$(printf '1\n%s\ns\n3\n' "$f" | $ZIPZ)
  if [ ! -f "$f.zipz" ]; then echo "$f: falha na compressao"; continue; fi
  sc=$(wc -c < "$f.zipz")
  tc=$(echo "$saida" | sed -n 's/.*Tempo de compressao: \([0-9.]*\) s.*/\1/p')
  saida=$(printf '2\n%s\nrest_%s\n3\n' "$f.zipz" "$f" | $ZIPZ)
  td=$(echo "$saida" | sed -n 's/.*Tempo de descompressao: \([0-9.]*\) s.*/\1/p')
  if cmp -s "$f" "rest_$f"; then ok=OK; else ok=DIFERENTE; fi

  t0=$(agora); gzip -c "$f" > "$f.gz"; t1=$(agora)
  gzip -dc "$f.gz" > "gzrest_$f"; t2=$(agora)
  cmp -s "$f" "gzrest_$f" || ok="$ok/gzip-DIFERENTE"
  sg=$(wc -c < "$f.gz")

  printf "%-10s %8s %6s %6s %-6s | %8s %6s %5ss %5ss | %8s %6s %5ss %5ss | %s\n" \
    "$f" "$so" "$h" "$rmin" "$cen" "$sc" "$(razao "$sc" "$so")" "$tc" "$td" \
    "$sg" "$(razao "$sg" "$so")" "$(diferenca "$t0" "$t1")" "$(diferenca "$t1" "$t2")" "$ok"
  rm -f "$f.zipz" "rest_$f" "$f.gz" "gzrest_$f"
done
