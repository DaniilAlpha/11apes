#include "pdp11/pdp11.h"

#include <stdio.h>

Result pdp11_init(Pdp11 *const self) {
    cpu_init(&self->cpu, PDP11_BOOT_ADDRESS, 0x0000);
    UNROLL(ram_init(&self->ram));

    return Ok;
}

void pdp11_step(Pdp11 *const self) {
    uint16_t const instruction =
        *(uint16_t *)(self->ram.byte + self->cpu.registers.r[7]++);
    printf("next instruction: %hu\n", instruction);
}

void pdp11_uninit(Pdp11 *const self) { ram_uninit(&self->ram); }
