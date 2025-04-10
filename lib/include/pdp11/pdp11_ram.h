#ifndef PDP11_RAM_H
#define PDP11_RAM_H

#include <stdint.h>

#include <result.h>

#include "pdp11/unibus/unibus.h"

// PDP-11 can address has 32K words of RAM, but top 4096 are reserved
// #define PDP11_ARRD_STACK_BOTTOM  (0400)
// #define PDP11_ARRD_PERIPH_BOTTOM (160000)

typedef struct Pdp11Ram {
    void *_ram;

    uint16_t _starting_addr, _size;
    bool _is_destructive_read;
} Pdp11Ram;

Result pdp11_ram_init(
    Pdp11Ram *const self,
    uint16_t const starting_addr,
    uint16_t const size,
    bool const is_destructive_read
);
void pdp11_ram_uninit(Pdp11Ram *const self);

UnibusDevice pdp11_ram_ww_unibus_device(Pdp11Ram *const self);

#endif
