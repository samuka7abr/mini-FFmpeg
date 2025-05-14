#include <stdint.h>
#include <limits.h>
#include "filters.h"

void apply_volume_c(int16_t *buffer, int sample_count, float volume) {
    const int32_t max = INT16_MAX;
    const int32_t min = INT16_MIN;
    for (int i = 0; i < sample_count; i++) {
        int32_t tmp = (int32_t)(buffer[i] * volume);
        if (tmp >  max) tmp =  max;
        else if (tmp < min) tmp = min;
        buffer[i] = (int16_t)tmp;
    }
}
//para teste
void apply_volume_asm(int16_t *buffer, int sample_count, float volume) {
    apply_volume_c(buffer, sample_count, volume);
}