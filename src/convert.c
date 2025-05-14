#define _POSIX_C_SOURCE 199309L
#include "convert.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>  // para fprintf

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

// contextos globais de input/output, decoder/encoder e resampler
static AVFormatContext *input_format_context  = NULL;
static AVFormatContext *output_format_context = NULL;
static AVCodecContext  *decoder_context       = NULL;
static AVCodecContext  *encoder_context       = NULL;
static SwrContext      *resampler_context     = NULL;

// estruturas de dados para áudio
static AVFrame  *decoded_frame         = NULL;
static AVPacket *packet                = NULL;
static int      audio_stream_index     = -1;
static int16_t *resampled_buffer       = NULL;
static int      resampled_sample_count = 0;

// variáveis de medição de performance
static long   accumulated_filter_time_nsec = 0;
static double accumulated_total_time_sec   = 0;

int open_input(const char *filename) {
    // abre o container de input
    int ret = avformat_open_input(&input_format_context, filename, NULL, NULL);
    if (ret < 0) return ret;

    // lê informações de streams (vídeo, áudio, legendas)
    ret = avformat_find_stream_info(input_format_context, NULL);
    if (ret < 0) return ret;

    // encontra índice do stream de áudio
    for (int i = 0; i < input_format_context->nb_streams; i++) {
        if (input_format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            break;
        }
    }
    if (audio_stream_index < 0) return -1;

    // prepara decoder de áudio
    const AVCodec *decoder = avcodec_find_decoder(
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

    // aloca estruturas de decodificação
    decoded_frame = av_frame_alloc();
    packet       = av_packet_alloc();

    // inicializa resampler para converter para S16 intercalado
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    resampler_context = swr_alloc_set_opts(
        NULL,
        AV_CH_LAYOUT_STEREO,
        AV_SAMPLE_FMT_S16,
        decoder_context->sample_rate,
        decoder_context->channel_layout,
        decoder_context->sample_fmt,
        decoder_context->sample_rate,
        0, NULL
    );
    #pragma GCC diagnostic pop
    swr_init(resampler_context);
    return 0;
}

int open_output(const char *filename) {
    // cria container de output forçando formato MP3
    int ret = avformat_alloc_output_context2(&output_format_context, NULL, "mp3", filename);
    if (ret < 0 || !output_format_context) {
        fprintf(stderr, "Erro ao criar output context para MP3.\n");
        return ret;
    }

    // configura encoder MP3 e adiciona stream ao container
    const AVCodec *encoder  = avcodec_find_encoder(AV_CODEC_ID_MP3);
    AVStream *output_stream = avformat_new_stream(output_format_context, encoder);
    encoder_context         = avcodec_alloc_context3(encoder);
    encoder_context->sample_rate = decoder_context->sample_rate;
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    encoder_context->channel_layout = AV_CH_LAYOUT_STEREO;
    encoder_context->channels       = 2;
    #pragma GCC diagnostic pop
    encoder_context->sample_fmt = AV_SAMPLE_FMT_S16P;  // planar compatível
    encoder_context->bit_rate   = 128000;

    ret = avcodec_open2(encoder_context, encoder, NULL);
    if (ret < 0) return ret;
    avcodec_parameters_from_context(
        output_stream->codecpar,
        encoder_context
    );

    // abre arquivo e escreve cabeçalho do container
    if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
        avio_open(&output_format_context->pb, filename, AVIO_FLAG_WRITE);
    }
    int err = avformat_write_header(output_format_context, NULL);
    (void)err;
    return 0;
}

int decode_frame(void) {
    // lê pacotes até encontrar áudio
    while (av_read_frame(input_format_context, packet) >= 0) {
        if (packet->stream_index != audio_stream_index) {
            av_packet_unref(packet);
            continue;
        }
        // mede tempo de decodificação
        struct timespec t_start, t_end;
        clock_gettime(CLOCK_MONOTONIC, &t_start);
        avcodec_send_packet(decoder_context, packet);
        int ret = avcodec_receive_frame(decoder_context, decoded_frame);
        clock_gettime(CLOCK_MONOTONIC, &t_end);
        accumulated_total_time_sec += diff_nsec(t_start, t_end) / 1e9;
        av_packet_unref(packet);
        if (ret < 0) continue;

        // calcula quantidade de samples de saída após resample
        int64_t delay = swr_get_delay(resampler_context, decoder_context->sample_rate);
        int out_samples = av_rescale_rnd(
            delay + decoded_frame->nb_samples,
            decoder_context->sample_rate,
            decoder_context->sample_rate,
            AV_ROUND_UP
        );
        // aloca buffer para samples convertidos
        av_samples_alloc((uint8_t**)&resampled_buffer,
                         NULL,
                         2,
                         out_samples,
                         AV_SAMPLE_FMT_S16,
                         0);
        // converte PCM para buffer de saída
        resampled_sample_count = swr_convert(
            resampler_context,
            (uint8_t**)&resampled_buffer,
            out_samples,
            (const uint8_t**)decoded_frame->data,
            decoded_frame->nb_samples
        );
        return 1; // buffer pronto para filtro
    }
    return 0; // sem mais frames
}

int encode_frame(void) {
    // prepara frame de saída planar a partir do buffer intercalado
    AVFrame *output_frame = av_frame_alloc();
    output_frame->nb_samples = resampled_sample_count;
    output_frame->format     = AV_SAMPLE_FMT_S16P;
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    output_frame->channel_layout = AV_CH_LAYOUT_STEREO;
    #pragma GCC diagnostic pop
    output_frame->sample_rate = decoder_context->sample_rate;
    av_frame_get_buffer(output_frame, 0);

    // divide buffer intercalado em canais planos
    int16_t *chan0 = (int16_t*)output_frame->data[0];
    int16_t *chan1 = (int16_t*)output_frame->data[1];
    for (int i = 0; i < resampled_sample_count; i++) {
        chan0[i] = resampled_buffer[2*i];
        chan1[i] = resampled_buffer[2*i + 1];
    }

    // mede tempo de codificação
    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    avcodec_send_frame(encoder_context, output_frame);
    int ret = avcodec_receive_packet(encoder_context, packet);
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    accumulated_total_time_sec += diff_nsec(t_start, t_end) / 1e9;

    // grava pacote codificado no container
    av_write_frame(output_format_context, packet);
    av_packet_unref(packet);
    av_frame_free(&output_frame);
    return ret >= 0;
}

void finalize_output(void) {
    // flush do encoder para processar pacotes pendentes
    avcodec_send_frame(encoder_context, NULL);
    while (avcodec_receive_packet(encoder_context, packet) == 0) {
        av_write_frame(output_format_context, packet);
        av_packet_unref(packet);
    }
    // escreve trailer e fecha container de saída
    av_write_trailer(output_format_context);
    if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
        avio_close(output_format_context->pb);
    }
    // libera contexts de encoder e decoder
    avcodec_free_context(&encoder_context);
    avcodec_free_context(&decoder_context);
    // libera frames, packet e resampler
    av_frame_free(&decoded_frame);
    av_packet_free(&packet);
    swr_free(&resampler_context);
    // libera contexts de input e output
    avformat_free_context(output_format_context);
    avformat_close_input(&input_format_context);
}

int16_t* get_buffer(void) {
    // retorna ponteiro para buffer resampled pronto
    return resampled_buffer;
}

int get_sample_count(void) {
    // retorna número de samples (canais x amostras)
    return resampled_sample_count * 2;
}

void add_filter_time(long nsec) {
    // acumula tempo gasto dentro do filtro de volume
    accumulated_filter_time_nsec += nsec;
}

double get_total_time(void) {
    // retorna tempo total (decodificação + codificação)
    return accumulated_total_time_sec;
}

double get_filter_time(void) {
    // retorna tempo gasto apenas no filtro
    return accumulated_filter_time_nsec / 1e9;
}

long diff_nsec(struct timespec start, struct timespec end) {
    // calcula diferença em nanosegundos entre dois timespecs
    return (end.tv_sec - start.tv_sec) * 1000000000L
         + (end.tv_nsec - start.tv_nsec);
}
