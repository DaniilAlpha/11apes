// http://www.bitsavers.org/pdf/dec/pdp11/1170/PDP-11_70_Handbook_1977-78.pdf

#include <stdio.h>

#include "pdp11/pdp11.h"
#include "pdp11/pdp11_rom.h"

long fsize(FILE *const file) {
    long const pos = ftell(file);
    fseek(file, 0, SEEK_END);
    long const size = ftell(file);
    fseek(file, pos, SEEK_SET);
    return size;
}

int main() {
    Pdp11 pdp = {0};
    assert(pdp11_init(&pdp) == Ok);

    Pdp11Rom rom = {0};
    FILE *const file = fopen("res/m9342-248f1.bin", "r");
    if (!file) return 1;
    assert(pdp11_rom_init_file(&rom, PDP11_STARTUP_PC, file) == Ok);
    fclose(file);
    pdp.unibus.devices[PDP11_FIRST_USER_DEVICE + 0] =
        pdp11_rom_ww_unibus_device(&rom);

    pdp11_start(&pdp);

    getc(stdin);

    pdp11_rom_uninit(&rom);

    pdp11_stop(&pdp);
    pdp11_uninit(&pdp);
}
