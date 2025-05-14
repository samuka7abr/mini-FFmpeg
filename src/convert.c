#define _POSIX_C_SOURCE 199309L
#include "convert.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

/*
PARA MELHOR LEITURA E COMPREENSÃO DO CÓDIGO:

Conceitos:
  - Container: invólucro do arquivo de mídia (mp4, mkv, mp3), organizando streams (vídeo, áudio, legendas).
  - Stream: cada fluxo dentro do container. Aqui usamos apenas o stream de áudio (audio_stream_index).
  - Codec: algoritmo de compressão/descompressão. codecpar define o tipo (AAC, MP3, etc.), usado para decodificar em PCM "cru".

Pacotes e Quadros:
  - AVPacket: dados codificados (compressos) lidos do arquivo.
  - AVFrame: dados decodificados em PCM (amostras de áudio).

Samples, Sample Rate e Channel Layout:
  - Sample: cada ponto no tempo da forma de onda. No PCM S16, cada sample é um int16_t.
  - Sample Rate: samples por segundo (ex.: 44100 Hz).
  - Channel Layout: disposição dos canais (mono, stereo, 5.1). Usamos AV_CH_LAYOUT_STEREO.

Resampling (SwrContext):
  - Converte formato de amostra (ex.: float → S16) e layout de canais ou taxa de amostragem.
  - Produz buffer (resampled_buffer) pronto para filtros ou encoder MP3.
*/

//contextos globais de input/output, decoder/encoder e resampler
static AVFormatContext *input_format_context = NULL;
static AVFormatContext *output_format_context = NULL;
static AVCodecContext  *decoder_context       = NULL;
static AVCodecContext  *encoder_context       = NULL;
static SwrContext      *resampler_context     = NULL;

//estruturas de dados para áudio
static AVFrame  *decoded_frame          = NULL;
static AVPacket *packet                 = NULL;
static int      audio_stream_index      = -1;
static int16_t *resampled_buffer        = NULL;
static int      resampled_sample_count  = 0;

//variáveis de medição de performance
static long   accumulated_filter_time_nsec = 0;
static double accumulated_total_time_sec   = 0;

int open_input(const char *filename) {
    //abre o container de input
    int ret = avformat_open_input(&input_format_context, filename, NULL, NULL);
    if (ret < 0) return ret;

    //lê informações de streams (vídeo, áudio, legendas)
    ret = avformat_find_stream_info(input_format_context, NULL);
    if (ret < 0) return ret;

    //encontra índice do stream de áudio
    for (int i = 0; i < input_format_context->nb_streams; i++) {
        if (input_format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            break;
        }
    }
    if (audio_stream_index < 0) return -1;

    //prepara decoder de áudio
    AVCodec *decoder = avcodec_find_decoder(
        input_format_context->streams[audio_stream_index]->codecpar->codec_id
    );
    if (!decoder) return -1;

    decoder_context = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(
        decoder_context,
        input_format_context->streams[audio_stream_index]->codecpar
    );
    ret = avcodec_open2(decoder_context, decoder, NULL);
    if (ret < 0) return ret;

    //aloca estruturas de decodificação
    decoded_frame = av_frame_alloc();
    packet       = av_packet_alloc();

    //inicializa resampler para converter para S16 stereo
    resampler_context = swr_alloc_set_opts(
        NULL,
        AV_CH_LAYOUT_STEREO,           //saída: stereo intercalado
        AV_SAMPLE_FMT_S16,             //saída: PCM 16-bit
        decoder_context->sample_rate,  //mesmo sample rate do input
        decoder_context->channel_layout,
        decoder_context->sample_fmt,
        decoder_context->sample_rate,
        0, NULL
    );
    swr_init(resampler_context);
    return 0;
}

int open_output(const char *filename) {
    //cria container de output deduzindo formato por extensão (.mp3)
    avformat_alloc_output_context2(&output_format_context, NULL, NULL, filename);

    //configura encoder MP3 e adiciona stream ao container
    AVCodec *encoder           = avcodec_find_encoder(AV_CODEC_ID_MP3);
    AVStream *output_stream    = avformat_new_stream(output_format_context, encoder);
    encoder_context            = avcodec_alloc_context3(encoder);
    encoder_context->sample_rate    = decoder_context->sample_rate;
    encoder_context->channel_layout = AV_CH_LAYOUT_STEREO;
    encoder_context->channels       = 2;
    encoder_context->sample_fmt     = AV_SAMPLE_FMT_S16;
    encoder_context->bit_rate       = 128000;

    avcodec_open2(encoder_context, encoder, NULL);
    avcodec_parameters_from_context(
        output_stream->codecpar,
        encoder_context
    );

    //abre arquivo e escreve cabeçalho do container
    if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
        avio_open(&output_format_context->pb, filename, AVIO_FLAG_WRITE);
    }
    avformat_write_header(output_format_context, NULL);
    return 0;
}

