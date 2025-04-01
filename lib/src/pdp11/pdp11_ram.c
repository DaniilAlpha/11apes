#include "pdp11/pdp11_ram.h"

#include <stdlib.h>

Result pdp11_ram_init(Pdp11Ram *const self) {
    void *const ram = malloc(PDP11_RAM_WORD_COUNT * sizeof(uint16_t));
    if (!ram) return OutOfMemErr;
    self->_ram = ram;

    return Ok;
}
void pdp11_ram_uninit(Pdp11Ram *const self) {
    free(self->_ram), self->_ram = NULL;
}
