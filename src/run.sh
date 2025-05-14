#!/usr/bin/env bash
set -euo pipefail

# -----------------------------------------------------------------------------
# Mini-FFmpeg-ASM Converter Run Script
#
# Projeto:       mini-ffmpeg-asm
# Descrição:     Extrai áudio de um vídeo, aplica filtro de volume (C ou ASM) e
#                gera MP3.
# Desenvolvido por: Samuel Abrão
# GitHub:        https://github.com/samuka7abr
# Portfolio:     https://portifolio-lyart-three-23.vercel.app/
# -----------------------------------------------------------------------------

print_help() {
    cat <<EOF
Mini-FFmpeg-ASM Converter Run Script

Projeto:       mini-ffmpeg-asm
Descrição:     Extrai áudio de um vídeo, aplica filtro de volume (C ou ASM) e gera MP3.
Desenvolvido por: Samuel Abrão
GitHub:        https://github.com/samuka7abr
Portfolio:     https://portifolio-lyart-three-23.vercel.app/

Uso: $(basename "$0") [OPÇÕES]

Opções:
  -i, --input PATH      Arquivo de vídeo de entrada
                        (padrão: ../videos/entrada.mp4)
  -o, --output PATH     Arquivo de áudio de saída (.mp3)
                        (padrão: ../audios/saida.mp3)
  -v, --volume VALOR    Fator de volume (float). 1.0 = nível original
                        (padrão: 1.0)
  -e, --engine [c|asm]  Filtro a usar: 'c' (C) ou 'asm' (Assembly SIMD)
                        (padrão: c)
  -h, --help            Exibe esta ajuda e sai

Exemplo:
  $(basename "$0") -i myvideo.mp4 -o out.mp3 -v 1.5 -e asm
EOF
}

# defaults
INPUT="../videos/entrada.mp4"
OUTPUT="../audios/saida.mp3"
VOLUME="1.0"
ENGINE="c"

# parse args
while [[ $# -gt 0 ]]; do
    case "$1" in
        -i|--input)  INPUT="$2";  shift 2;;
        -o|--output) OUTPUT="$2"; shift 2;;
        -v|--volume) VOLUME="$2"; shift 2;;
        -e|--engine) ENGINE="$2"; shift 2;;
        -h|--help)   print_help; exit 0;;
        *)
            echo "Opção desconhecida: $1" >&2
            print_help >&2
            exit 1;;
    esac
done

# validate engine
if [[ "$ENGINE" != "c" && "$ENGINE" != "asm" ]]; then
    echo "Engine inválido: '$ENGINE'. Use 'c' ou 'asm'." >&2
    exit 1
fi

# scripts directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$SCRIPT_DIR"

cd "$SRC_DIR" || exit 1

# if ASM mode, assemble the filter and enable stub exclusion
CFLAGS="-std=gnu11 -D_POSIX_C_SOURCE=199309L"
SOURCES="main.c convert.c filters/volume.c"
if [[ "$ENGINE" == "asm" ]]; then
    if ! command -v nasm &>/dev/null; then
        echo "Erro: 'nasm' não encontrado. Instale-o para compilar o filtro ASM." >&2
        exit 1
    fi
    echo "Montando filtro ASM..."
    nasm -f elf64 filters/volume.s -o filters/volume_asm.o
    CFLAGS+=" -DUSE_ASM"
    SOURCES+=" filters/volume_asm.o"
fi

echo "Compilando projeto..."
if gcc $CFLAGS -o exec $SOURCES \
       $(pkg-config --cflags --libs libavformat libavcodec libswresample libavutil)
then
    echo "Compilação concluída."
else
    echo "Falha na compilação." >&2
    exit 1
fi

mkdir -p "$(dirname "$OUTPUT")"

echo "Convertendo:"
echo "  Input : $INPUT"
echo "  Output: $OUTPUT"
echo "  Volume: $VOLUME"
echo "  Engine: $ENGINE"

if ./exec "$INPUT" "$OUTPUT" "$VOLUME" "$ENGINE"; then
    echo "Conversão concluída com sucesso."
else
    echo "Erro durante a conversão." >&2
    exit 1
fi

if [[ -s "$OUTPUT" ]]; then
    SIZE=$(stat -c '%s' "$OUTPUT")
    echo "Arquivo de saída: $OUTPUT ($SIZE bytes)"
else
    echo "Arquivo de saída não foi criado ou está vazio." >&2
    exit 1
fi

echo "Processo finalizado."
