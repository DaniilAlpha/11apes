#ifndef PDP11_RAM_H
#define PDP11_RAM_H

#include <stdint.h>

#include <result.h>

#include "pdp11/unibus/unibus_device.h"

// PDP-11 can address has 32K words of RAM, but top 4K are reserved for periphs
#define PDP11_RAM_MAX_SIZE ((32 - 4) * 1024 * 2)

typedef struct Pdp11Ram {
    void volatile *_data;

    uint16_t _starting_addr, _size;

    char const *_filepath;
} Pdp11Ram;

// Initializes the PDP-11 RAM. If `filepath` is `NULL`, RAM is considered
// volatile, otherwise it is saved to the provided file.
Result pdp11_ram_init(
    Pdp11Ram *const self,
    uint16_t const starting_addr,
    uint16_t const size,
    char const *const filepath
);
void pdp11_ram_uninit(Pdp11Ram *const self);

Result pdp11_ram_save(Pdp11Ram *const self);
Result pdp11_ram_load(Pdp11Ram *const self);

UnibusDevice pdp11_ram_ww_unibus_device(Pdp11Ram *const self);

#endif
