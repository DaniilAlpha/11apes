#include "pdp11/pdp11_ram.h"

#include <stdlib.h>

#include <assert.h>

#include "woodi.h"

static bool pdp11_ram_try_read(
    Pdp11Ram *const self,
    uint16_t const addr,
    uint16_t *const out_val
) {
    if (!(self->_starting_addr <= addr &&
          addr < self->_starting_addr + self->_size))
        return false;

    uint16_t *const data_ptr = (uint16_t *)(self->_data + addr);
    *out_val = *data_ptr;

    return true;
}
static bool pdp11_ram_try_write_word(
    Pdp11Ram *const self,
    uint16_t const addr,
    uint16_t const val
) {
    if (!(self->_starting_addr <= addr &&
          addr < self->_starting_addr + self->_size))
        return false;

    *(uint16_t *)(self->_data + addr) = val;

    return true;
}
static bool pdp11_ram_try_write_byte(
    Pdp11Ram *const self,
    uint16_t const addr,
    uint8_t const val
) {
    if (!(self->_starting_addr <= addr &&
          addr < self->_starting_addr + self->_size))
        return false;

    *(uint8_t *)(self->_data + addr) = val;

    return true;
}

Result pdp11_ram_init(
    Pdp11Ram *const self,
    uint16_t const starting_addr,
    uint16_t const size
) {
    assert(size <= PDP11_RAM_MAX_SIZE);

    void *const data = malloc(size);
    if (!data) return OutOfMemErr;
    self->_data = data;

    self->_starting_addr = starting_addr;
    self->_size = size;

    return Ok;
}
void pdp11_ram_uninit(Pdp11Ram *const self) {
    free((void *)self->_data), self->_data = NULL;

    self->_starting_addr = self->_size = 0;
}

UnibusDevice pdp11_ram_ww_unibus_device(Pdp11Ram *const self) {
    WRAP_BODY(
        UnibusDevice,
        UNIBUS_DEVICE_INTERFACE(Pdp11Ram),
        {
            ._try_read = pdp11_ram_try_read,
            ._try_write_word = pdp11_ram_try_write_word,
            ._try_write_byte = pdp11_ram_try_write_byte,
        }
    );
}
