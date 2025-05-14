#!/usr/bin/env bash
set -euo pipefail

sudo apt-get update
sudo apt-get install -y \
  gcc make pkg-config \
  libavformat-dev libavcodec-dev libswresample-dev libavutil-dev \
  nasm

mkdir -p videos audios

cd src

cat <<EOF

Setup concluído.
1) Copie seu vídeo para: $(dirname "$PWD")/videos/
2) Execute: ./run.sh
3) Para ver todas as opções, use: ./run.sh --help

EOF
