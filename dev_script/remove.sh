#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Procurando e removendo todos os arquivos chamados 'exec' em $PROJECT_ROOT..."

find "$PROJECT_ROOT" -type f -name exec -exec rm -f {} +

echo "Concluído."
