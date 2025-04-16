#ifndef PDP11_RAM_H
#define PDP11_RAM_H

#include <stdint.h>

#include <result.h>

#include "pdp11/unibus/unibus_device.h"

// PDP-11 can address has 32K words of RAM, but top 4K are reserved for periphs
#define PDP11_RAM_MAX_SIZE ((32 - 4) * 1024)

#define PDP11_RAM_FILEPATH (".ram")

typedef struct Pdp11Ram {
    // NOTE you may argue that volatile is excessive, but it won't hurt anyway
    void volatile *_data;

    uint16_t _starting_addr, _size;

    bool _is_volatile;
} Pdp11Ram;

Result pdp11_ram_init(
    Pdp11Ram *const self,
    uint16_t const starting_addr,
    uint16_t const size,
    bool const is_volatile
);
void pdp11_ram_uninit(Pdp11Ram *const self);

UnibusDevice pdp11_ram_ww_unibus_device(Pdp11Ram *const self);

#endif
