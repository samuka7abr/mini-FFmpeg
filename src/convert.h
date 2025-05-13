#ifndef CONVERT_H
#define CONVERT_H

#include <stdint.h>
#include <time.h>

typedef struct AVFormatContext AVFormatContext;
typedef struct AVCodecContext AVCodecContext;
typedef struct SwrContext SwrContext;
typedef struct AVPacket AVPacket;
typedef struct AVFrame AVFrame;

int open_input(const char *filename);

int open_output(const char *filename);

int decode_frame(void);

int encode_frame(void);

void finalize_output(void);

int16_t* get_buffer(void);

int get_sample_count(void);

void add_filter_time(long nsec);

double get_total_time(void);

double get_filter_time(void);

long diff_nsec(struct timespec start, struct timespec end);

#endif 
