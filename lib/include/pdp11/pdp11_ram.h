#ifndef PDP11_RAM_H
#define PDP11_RAM_H

#include <stdint.h>

#include <result.h>

#include "pdp11/unibus/unibus.h"

// PDP-11 can address has 32K words of RAM, but top 4096 are reserved
#define PDP11_RAM_SIZE ((32 * 1024 - 4096) * sizeof(uint16_t))
// #define PDP11_ARRD_STACK_BOTTOM  (0400)
// #define PDP11_ARRD_PERIPH_BOTTOM (160000)

typedef struct Pdp11Ram {
    void *_ram;
} Pdp11Ram;

Result pdp11_ram_init(Pdp11Ram *const self);
void pdp11_ram_uninit(Pdp11Ram *const self);

UnibusDevice pdp11_ram_ww_unibus_device(Pdp11Ram *const self);

#endif
