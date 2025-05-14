#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/src"

gcc -std=gnu11 -D_POSIX_C_SOURCE=199309L \
    -o test_convert \
    test_convert.c convert.c \
    $(pkg-config --cflags --libs libavformat libavcodec libswresample libavutil)

INPUT="${1:-../videos/entrada.mp4}"
OUTPUT="${2:-saida.mp3}"

./test_convert "$INPUT" "$OUTPUT"
