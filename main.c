// http://www.bitsavers.org/pdf/dec/pdp11/1170/PDP-11_70_Handbook_1977-78.pdf

#include <stdio.h>
#include <stdlib.h>

#include "pdp11/pdp11.h"

long fsize(FILE *const file) {
    long const pos = ftell(file);
    fseek(file, 0, SEEK_END);
    long const size = ftell(file);
    fseek(file, pos, SEEK_SET);
    return size;
}

int main() {
    Pdp11 pdp = {0};
    UNROLL(pdp11_init(&pdp));

    FILE *const file = fopen("res/m9342-248f1.bin", "r");
    if (!file) return 1;
    size_t const file_data_size = fsize(file);
    uint8_t *const file_data = malloc(file_data_size);
    if (!file_data) return 1;
    fread(file_data, 1, file_data_size, file);

    assert((file_data_size & 1) == 0);
    for (size_t i = 0; i < file_data_size; i += 2) {
        unibus_dato(
            &pdp.unibus,
            PDP11_STARTUP_PC + i,
            (uint16_t)(file_data[i] << 8) | (uint8_t)(file_data[i + 1] >> 8)
        );
    }

    free(file_data);
    fclose(file);

    pdp11_start(&pdp);

    getc(stdin);

    pdp11_stop(&pdp);
    pdp11_uninit(&pdp);
}
