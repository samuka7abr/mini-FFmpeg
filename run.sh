#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src"

cd "$SRC_DIR"

echo "=== Compilando o conversor mini-FFmpeg ==="
if gcc -std=gnu11 -D_POSIX_C_SOURCE=199309L \
    -o test_convert \
    test_convert.c convert.c \
    $(pkg-config --cflags --libs libavformat libavcodec libswresample libavutil)
then
    echo "✔️  Compilação bem-sucedida"
else
    echo "❌ Falha na compilação" >&2
    exit 1
fi

INPUT="${1:-../videos/entrada.mp4}"
OUTPUT="${2:-../audios/saida.mp3}"

echo
echo "=== Executando conversão ==="
echo "Input:  $INPUT"
echo "Output: $OUTPUT"

OUTPUT_DIR="$(dirname "$OUTPUT")"
mkdir -p "$OUTPUT_DIR"

if ./test_convert "$INPUT" "$OUTPUT"
then
    echo "✔️  Conversão finalizada com sucesso"
else
    echo "❌ Erro durante a conversão" >&2
    exit 1
fi

if [[ -s "$OUTPUT" ]]
then
    echo "✔️  Arquivo de saída encontrado: $OUTPUT"
    echo "Tamanho: $(stat -c '%s' "$OUTPUT") bytes"
else
    echo "❌ Arquivo de saída não foi criado ou está vazio" >&2
    exit 1
fi

echo
echo "Tudo pronto!"
