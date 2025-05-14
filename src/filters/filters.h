#ifndef FILTERS_H
#define FILTERS_H
#include <stdint.h>
void apply_volume_c(int16_t *buffer, int sample_count, float volume);
void apply_volume_asm(int16_t *buffer, int sample_count, float volume);
#endif
