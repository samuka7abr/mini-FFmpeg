# mini-FFmpeg

> **Estudo de Processamento de Áudio em C e Assembly SIMD**

---

## 📖 Descrição

O **mini-FFmpeg** é um projeto “hands-on” voltado ao aprendizado de:

* Manipulação de containers e streams multimídia com **FFmpeg** (libavformat, libavcodec, libswresample, libavutil)
* Processamento de áudio em **C puro** e **Assembly SIMD** (SSE2/AVX)
* Integração de código C e Assembly no mesmo pipeline
* Medição de performance de rotinas de processamento (benchmark em tempo real)
* Criação de uma **CLI** simples para converter vídeo em MP3 com filtro de volume

É ideal para quem deseja entender de forma prática como bibliotecas de baixo nível funcionam, como otimizar código de processamento digital de sinais (DSP) e como ligar rotinas em Assembly dentro de um projeto em C.

---

## 🚀 Tecnologias Utilizadas

* **Linguagens:** C, Assembly x86-64 (NASM)
* **Bibliotecas:**

  * libavformat – leitura/escrita de containers (MP4, MP3…)
  * libavcodec   – codificação e decodificação de áudio
  * libswresample – conversão de formatos e taxas de amostragem
  * libavutil     – utilitários de buffer, tipos e I/O
* **Ferramentas:**

  * **GCC** (GNU Compiler Collection)
  * **NASM** (Netwide Assembler)
  * **pkg-config**
* **Ambiente:** Linux (Debian/Ubuntu)

---

## ⚙️ Pré-requisitos

Instale as dependências:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential gcc pkg-config make \
  libavformat-dev libavcodec-dev libswresample-dev libavutil-dev \
  nasm
```

---

## 🛠️ Instalação e Setup

1. **Clone o repositório**

   ```bash
   git clone https://github.com/samuka7abr/mini-FFmpeg.git
   cd mini-FFmpeg
   ```

2. **Execute o script de setup**

   ```bash
   chmod +x build.sh
   ./build.sh
   ```

3. **Adicione seu vídeo de entrada**
   Copie o arquivo `entrada.mp4` (ou outro `.mp4`, `.webm` etc.) para:

   ```text
   mini-FFmpeg/
   └── videos/entrada.mp4
   ```

---

## 📂 Estrutura de Diretórios

```text
mini-FFmpeg/
├──dev_script
    ├── git-commit.sh ← script para automação de commits
    ├── remove.sh     ← script para remover executáveis
├── build.sh          ← script de setup
├── videos/           ← coloque seus vídeos aqui
├── audios/           ← o MP3 gerado aparecerá aqui
└── src/
    ├── convert.c
    ├── convert.h
    ├── main.c
    ├── run.sh        ← script de execução e conversão
    └── filters/
        ├── filters.h
        ├── volume.c
        └── volume.s  ← filtro SIMD em Assembly
```

---

## ▶️ Uso Básico

1. **Dentro de `mini-FFmpeg/`**, execute o script de conversão:

   ```bash
   ./src/run.sh
   ```

2. **Padrões**

   * **Entrada:** `videos/entrada.mp4`
   * **Saída:**  `audios/saida.mp3`
   * **Volume:** `1.0`
   * **Engine:** `c`

3. **Opções**
   Para ver todas as flags:

   ```bash
   ./src/run.sh --help
   ```

4. **Exemplo com Assembly e volume**

   ```bash
   ./src/run.sh -i ../videos/entrada.mp4 \
                -o ../audios/entrada_aumentada.mp3 \
                -v 1.5 \
                -e asm
   ```

---

## 📚 Bibliografia & Referências

1. **FFmpeg Documentation**

   * [https://www.ffmpeg.org/documentation.html](https://www.ffmpeg.org/documentation.html)
2. **NASM Manual**

   * [https://nasm.us/doc/](https://nasm.us/doc/)
3. **Intel 64 and IA-32 Architectures Software Developer’s Manual**

   * Volumes 1–3 (SSE2/AVX intrinsics)
4. **Digital Signal Processing (DSP)**

   * Conceitos de PCM, sample rate, channel layout, saturação (clipping)
5. **System V AMD64 ABI**

   * Convenção de chamadas (passagem de registradores, stack frame)

---

## ✍️ Autor

**Samuel Abrão**

* GitHub: [https://github.com/samuka7abr](https://github.com/samuka7abr)
* Portfólio: [https://portifolio-lyart-three-23.vercel.app/](https://portifolio-lyart-three-23.vercel.app/)

Projeto desenvolvido para estudo e aperfeiçoamento em tratamento de arquivos multimídia, integração C ↔ Assembly e otimização de DSP em baixo nível.
