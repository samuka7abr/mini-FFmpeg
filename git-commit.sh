#!/bin/bash
set -e

echo ""
echo "📁 Diretório atual: $(pwd)"

# Exibe branch atual
current_branch=$(git symbolic-ref --short HEAD 2>/dev/null || echo "HEAD (detached)")
echo "🌿 Branch atual: $current_branch"
echo ""

echo "Selecione o tipo de commit:"
echo "1 - feat     (Nova funcionalidade)"
echo "2 - chore    (Tarefa técnica / build)"
echo "3 - fix      (Correção de bug)"
echo "4 - delete   (Remoção de código)"
echo "5 - docs     (Documentação)"
echo "6 - refactor (Refatoração de código)"
echo "7 - build    (Alterações no sistema de build)"
echo ""

read -p "Digite o número correspondente: " tipo

case $tipo in
  1) prefixo="feat" ;;
  2) prefixo="chore" ;;
  3) prefixo="fix" ;;
  4) prefixo="delete" ;;
  5) prefixo="docs" ;;
  6) prefixo="refactor" ;;
  7) prefixo="build" ;;
  *) echo "❌ Tipo inválido"; exit 1 ;;
esac

echo ""
read -p "Mensagem do commit: " msg

if [ -z "$msg" ]; then
  echo "❌ A mensagem do commit não pode estar vazia."
  exit 1
fi

echo ""
echo "🚀 Commitando com: $prefixo: $msg"
echo ""

git add .
git commit -m "$prefixo: $msg" || {
  echo "❌ Erro ao tentar commitar. Verifique se há validações ou hooks ativos."
  exit 1
}

git push && echo "✅ Push feito com sucesso!"
