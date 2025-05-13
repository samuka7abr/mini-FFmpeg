#include "convert.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

/*
PARA MELHOR LEITURA E COMPREENSÃO DO C´ODIGO:
Conceitos:
    containers: invólucro do arquivo de mídia (mp4, mkv, mp3)
                organiza streams (fluxos) de dados (vídeo, audio, legendas)

    stream: cada fluxo dentro desse container. no caso dessa aplicação,
            apenas o stream de áudio está sendo usado (audio_stream_index)

    codec: algorítmo de compressão/descompressão. Cada um dos streams possui
            um codecpar (codec parameters) que diz qual tipo de codec usar
            (AAC, MP3, VORBIS, etc) (quando usado no for da função open_input,
            faz o papel de encontrar qual o melhor tipo de decode pra 
            transformar o áudio comprimido em PCM "cru")

Pacotes e Quadros (AVPacket e AVFrame):
    AVPacket: contém dados compressos, exatamente como saíram do arquivo. 

    AVFrame: resultado da descodificação: amostras PCM que representam a forma
            de ondas de audio em valores inteiros ou float.

Samples, Sample Rate e Channel Layout:
    Sample: cada ponto no tempo da forma de onda no áudio. no PCM 16bits (S16),
            cada sample é um int16_t que diz o "quão alto" é o som naquele instante.
    
    Sample Rate: número de samples por segundo (ex: 44.1kHz = 44.100 samples/s). 
                quanto maior, maior a fidelidade, mas mais dados.
    
    Channel Layout: determina a posição dos canais (mono, estereo, 5.1, etc) 
                    AV_CH_LAYOUT_STEREO significa dois canais lado a lado

Resampling(SwrContext):
    SrwContext: faz duas funções: 
                                  converter o formato de sample
                                  por ex: float,S16P -> S16 intercalado
                                
                                  converter Channel Layout
                                  ou mudar a taxa de amostragem, se preciso
                ele recebe o PCM bruto e converte para o buffer (resampled_buffer) pronto
                para filtro ou encoder MP3    
*/

//entrada e saída (mp4, mp3)
static AVFormatContext *input_format_context = NULL;
static AVFormatContext *output_format_context = NULL;
//contextos para leitura de PCM e codificação de mp3
static AVCodecContext *decoder_context = NULL;
static AVCodecContext *encoder_context = NULL;
//conversão de audio para formatos e canais
static SwrContext *resampler_context = NULL;
//tratamento de audio bruto
static AVFrame *decoded_frame = NULL;
static AVPacket *packet = NULL;
static int audio_stream_index = -1;
//contém os samples pós resample
static int16_t *resampled_buffer = NULL;
static int resampled_sample_count = 0;
//variáveis de performance
static long accumulated_filter_time_nsec = 0;
static double accumulated_total_time_sec = 0;

int open_input(const char *filename){
    //abre o container
    int ret = avformat_open_input(&input_format_context, filename, NULL, NULL);
    if(ret < 0) return ret;
    
    //carrega o arquivo lendo as infos de stream(video, audio, legendas, etc)
    ret = avformat_find_stream_info(input_format_context, NULL);
    if(ret < 0) return ret;
    
    //percorre o formato de input até encontrar aquele cujo o tipo de codec == AVMEDIA_TYPE_AUDIO
    for(int i = 0; i < input_format_context->nb_streams; i++){
        if(input_format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            //guarda o index do codec i
            audio_stream_index = i;
            break;
        }
    }

    if(audio_stream_index < 0) return -1;

    //atribui a decoder o decodificador mais adequado ao codec detectado
    AVCodec *decoder = avcodec_find_decoder(
        input_format_context->streams[audio_stream_index]->codecpar->codec_id);
    if(!decoder) return -1;
    
    //preenchimento de contexto com parâmetros da stream
    decoder_context = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(
        decoder_context,
        input_format_context->streams[audio_stream_index]->codecpar
    );
    
    //abre o decoder pra uso
    ret = avcodec_open2(decoder_context, decoder, NULL);
    if(ret < 0) return ret;

    //recebe PCM e lê pacotes, respectivamente
    decoded_frame = av_frame_alloc();
    packet = av_packet_alloc();

    resampler_context = swr_alloc_set_opts( //inicialida resampler definindo:
        NULL,
        //formato de saída
        AV_CH_LAYOUT_STEREO, //estéreo
        AV_SAMPLE_FMT_S16, //16bits inteiros, taxa igual a do input

        //formato de entrada extraído de decoder_context
        decoder_context->sample_rate,
        decoder_context->channel_layout,
        decoder_context->sample_fmt,
        decoder_context->sample_rate,
        0,
        NULL
    );

    swr_init(resampler_context); //ativando resampler
    return 0;
}