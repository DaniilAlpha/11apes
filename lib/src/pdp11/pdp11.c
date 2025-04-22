#include "pdp11/pdp11.h"

#include <pthread.h>

#include <unistd.h>

Result pdp11_init(Pdp11 *const self) {
    UNROLL(pdp11_ram_init(&self->ram, 0, PDP11_RAM_SIZE, "core.ram"));

    unibus_init(&self->unibus, &self->cpu);
    self->unibus.devices[0] = pdp11_ram_ww_unibus_device(&self->ram);

    pdp11_cpu_init(&self->cpu, &self->unibus);

    return Ok;
}
void pdp11_uninit(Pdp11 *const self) {
    pdp11_cpu_uninit(&self->cpu);
    unibus_uninit(&self->unibus);
    pdp11_ram_uninit(&self->ram);
}
