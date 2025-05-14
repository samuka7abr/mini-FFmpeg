#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src"

cd "$SRC_DIR" || exit 1

echo "=== Compilando o conversor mini-FFmpeg ==="
if gcc -std=gnu11 -D_POSIX_C_SOURCE=199309L \
    -o exec \
    main.c convert.c \
    filters/volume.c \
    $(pkg-config --cflags --libs libavformat libavcodec libswresample libavutil)
then
    echo "Compilação bem-sucedida"
else
    echo "Falha na compilação" >&2
    exit 1
fi

INPUT="${1:-../videos/entrada.mp4}"
OUTPUT="${2:-../audios/saida.mp3}"
VOLUME="${3:-1.0}"
ENGINE="${4:-c}"

echo
echo "=== Executando conversão ==="
echo "Input:  $INPUT"
echo "Output: $OUTPUT"
echo "Volume: $VOLUME"
echo "Engine: $ENGINE"

mkdir -p "$(dirname "$OUTPUT")"

if ./exec "$INPUT" "$OUTPUT" "$VOLUME" "$ENGINE"; then
    echo "Conversão concluída com sucesso"
else
    echo "Erro durante a conversão" >&2
    exit 1
fi

if [[ -s "$OUTPUT" ]]; then
    echo "Arquivo de saída gerado: $OUTPUT"
    echo "Tamanho: $(stat -c '%s' "$OUTPUT") bytes"
else
    echo "Arquivo de saída não foi criado ou está vazio" >&2
    exit 1
fi

echo
echo "Processo concluído."
