// http://www.bitsavers.org/pdf/dec/pdp11/1120/

#include <stdio.h>

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
    fread(&pdp11_ram_byte_at(&pdp, 0), 1, fsize(file), file);
    for (uint16_t *word_ptr = &pdp11_ram_word_at(&pdp, 0);
         word_ptr <= &pdp11_ram_word_at(&pdp, PDP11_RAM_WORD_COUNT - 1);
         word_ptr++)
        *word_ptr = (uint16_t)(*word_ptr << 8) | (uint8_t)(*word_ptr >> 8);
    for (long i = 0; i < fsize(file); i++) {
        pdp11_step(&pdp);
        while (getchar() != '\n');
    }
    fclose(file);

    pdp11_uninit(&pdp);
}
