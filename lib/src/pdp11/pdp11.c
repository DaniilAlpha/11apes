#include "pdp11/pdp11.h"

#include <pthread.h>
#include <stdio.h>

#include <unistd.h>

Result pdp11_init(Pdp11 *const self) {
    unibus_init(&self->unibus, &self->cpu);

    UNROLL_CLEANUP(pdp11_ram_init(&self->ram, 0, PDP11_RAM_SIZE, "core.ram"), {
        unibus_uninit(&self->unibus);
    });

    UNROLL_CLEANUP(pdp11_cpu_init(&self->cpu, &self->unibus), {
        pdp11_ram_uninit(&self->ram);
        unibus_uninit(&self->unibus);
    });

    self->periphs = self->unibus.devices;

    self->periphs++[0] = pdp11_ram_ww_unibus_device(&self->ram);

    return Ok;
}
void pdp11_uninit(Pdp11 *const self) {
    pdp11_cpu_uninit(&self->cpu);
    pdp11_ram_uninit(&self->ram);
    unibus_uninit(&self->unibus);
}
