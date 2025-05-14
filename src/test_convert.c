#define _POSIX_C_SOURCE 199309L
#include "convert.h"
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <entrada> <saida>\n", argv[0]);
        return 1;
    }
    if (open_input(argv[1]) < 0) {
        fprintf(stderr, "Erro ao abrir input: %s\n", argv[1]);
        return 1;
    }
    if (open_output(argv[2]) < 0) {
        fprintf(stderr, "Erro ao abrir output: %s\n", argv[2]);
        return 1;
    }
    while (decode_frame()) {
        encode_frame();
    }
    finalize_output();
    printf("Total: %.3f s | Filtro: %.3f s\n", get_total_time(), get_filter_time());
    return 0;
}
