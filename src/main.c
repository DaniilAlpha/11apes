#include <stdio.h>

#include "pdp11/core/cpu.h"
#include "pdp11/core/ram.h"
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
    if (pdp11_init(&pdp) != Ok) return 1;

    FILE *const file = fopen("res/m9342-248f1.bin", "r");
    fread(pdp.ram.byte + PDP11_BOOT_ADDRESS, 1, fsize(file), file);
    for (long i = 0; i < fsize(file); i++) pdp11_step(&pdp);
    fclose(file);

    pdp11_uninit(&pdp);
}
