#ifndef PDP_11_H
#define PDP_11_H

#include "pdp11/core/cpu.h"
#include "pdp11/core/ram.h"

#define PDP11_BOOT_ADDRESS (0x8000)

typedef struct Pdp11 {
    Cpu cpu;
    Ram ram;
} Pdp11;

Result pdp11_init(Pdp11 *const self);
void pdp11_step(Pdp11 *const self);

void pdp11_uninit(Pdp11 *const self);

#endif
