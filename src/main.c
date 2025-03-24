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

    pdp.cpu.r[0] = 24, pdp.cpu.r[1] = 42;
    pdp11_write(&pdp, pdp.cpu.r[7], 0074100 /* xor R0, R1 */);
    pdp11_step(&pdp);
    printf("%i\n", pdp.cpu.r[0]);

    /* FILE *const file = fopen("res/m9342-248f1.bin", "r");
    fread((void *)pdp.ram + PDP11_STARTUP_PC, 1, fsize(file), file);
    for (uint16_t *word_ptr = pdp.ram;
         word_ptr < pdp.ram + PDP11_RAM_WORD_COUNT;
         word_ptr++)
        *word_ptr = (uint16_t)(*word_ptr << 8) | (uint8_t)(*word_ptr >> 8);
    for (long i = 0; i < fsize(file); i++) pdp11_step(&pdp);
    fclose(file); */

    pdp11_uninit(&pdp);
}
