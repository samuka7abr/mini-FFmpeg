#define _POSIX_C_SOURCE 199309L
#include "convert.h"
#include "filters.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <entrada> <saida> <volume> <c|asm>\n", argv[0]);
        return 1;
    }
    const char *infile   = argv[1];
    const char *outfile  = argv[2];
    float volume         = strtof(argv[3], NULL);
    char  engine         = argv[4][0];
    void (*filter_fn)(int16_t*, int, float) =
        (engine == 'a') ? apply_volume_asm : apply_volume_c;

    if (open_input(infile) < 0 || open_output(outfile) < 0) {
        fprintf(stderr, "Erro ao abrir arquivos\n");
        return 1;
    }

    struct timespec t0, t1;
    while (decode_frame()) {
        clock_gettime(CLOCK_MONOTONIC, &t0);
        filter_fn(get_buffer(), get_sample_count(), volume);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        add_filter_time(diff_nsec(t0, t1));
        encode_frame();
    }

    finalize_output();
    printf("Total: %.3f s | Filtro: %.3f s\n",
           get_total_time(), get_filter_time());
    return 0;
}
