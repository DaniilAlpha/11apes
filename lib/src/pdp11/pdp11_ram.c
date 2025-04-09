#include "pdp11/pdp11_ram.h"

#include <stdlib.h>

#include "woodi.h"

static uint16_t *
pdp11_ram_try_address(Pdp11Ram *const self, uint16_t const addr) {
    if (addr >= PDP11_RAM_SIZE) return NULL;
    return (uint16_t *)(self->_ram + addr);
}
// static inline uint8_t *
// pdp11_ram_byte(Pdp11Ram *const self, uint16_t const addr) {
//     if (addr >= PDP11_RAM_SIZE) return NULL;
//     return (uint8_t *)(self->_ram + addr);
// }

Result pdp11_ram_init(Pdp11Ram *const self) {
    void *const ram = malloc(PDP11_RAM_SIZE);
    if (!ram) return OutOfMemErr;
    self->_ram = ram;

    return Ok;
}
void pdp11_ram_uninit(Pdp11Ram *const self) {
    free(self->_ram), self->_ram = NULL;
}

UnibusDevice pdp11_ram_ww_unibus_device(Pdp11Ram *const self) {
    WRAP_BODY(
        UnibusDevice,
        UNIBUS_DEVICE_INTERFACE(Pdp11Ram),
        {._try_address = pdp11_ram_try_address}
    );
}
