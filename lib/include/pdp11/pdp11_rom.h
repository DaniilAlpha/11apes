#ifndef PDP11_ROM_H
#define PDP11_ROM_H

#include <stdint.h>
#include <stdio.h>

#include <result.h>

#include "pdp11/unibus/unibus_device.h"

typedef struct Pdp11Rom {
    void const *_data;

    uint16_t _starting_addr, _size;
} Pdp11Rom;

Result pdp11_rom_init_file(
    Pdp11Rom *const self,
    uint16_t const starting_addr,
    FILE *const file
);
void pdp11_rom_uninit(Pdp11Rom *const self);

UnibusDevice pdp11_rom_ww_unibus_device(Pdp11Rom *const self);

#endif
