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
    fread((void *)pdp.ram + PDP11_STARTUP_PC, 1, fsize(file), file);
    for (long i = 0; i < fsize(file); i++) pdp11_step(&pdp);
    fclose(file);

    pdp11_uninit(&pdp);
}
