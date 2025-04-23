#include "pdp11/pdp11_rom.h"

#include <stdlib.h>

#include <assert.h>
#include <woodi.h>

static long fsize(FILE *const self) {
    long const cur = ftell(self);
    fseek(self, 0, SEEK_END);
    long const size = ftell(self);
    fseek(self, cur, SEEK_SET);
    return size;
}

Result pdp11_rom_init_file(
    Pdp11Rom *const self,
    uint16_t const starting_addr,
    FILE *const file
) {
    size_t const size = fsize(file);
    if ((size & 1) != 0) return RangeErr;

    void *const data = malloc(size);
    if (!data) return OutOfMemErr;
    if (fread(data, size, 1, file) != 1) return FileReadingErr;
    self->_data = data;

    self->_starting_addr = starting_addr;
    self->_size = size;

    return Ok;
}
void pdp11_rom_uninit(Pdp11Rom *const self) {
    free((void *)self->_data), self->_data = NULL;

    self->_starting_addr = self->_size = 0;
}

/***************
 ** interface **
 ***************/

static void pdp11_rom_reset(Pdp11Rom *const) {}
static bool pdp11_rom_try_read(
    Pdp11Rom *const self,
    uint16_t addr,
    uint16_t *const out_val
) {
    addr -= self->_starting_addr;
    if (!(addr < self->_size)) return false;

    *out_val = *(uint16_t *)(self->_data + addr);

    return true;
}
static bool
pdp11_rom_try_write_word(Pdp11Rom *const self, uint16_t addr, uint16_t const) {
    addr -= self->_starting_addr;
    return addr < self->_size;
}
static bool
pdp11_rom_try_write_byte(Pdp11Rom *const self, uint16_t addr, uint8_t const) {
    addr -= self->_starting_addr;
    return addr < self->_size;
}
UnibusDevice pdp11_rom_ww_unibus_device(Pdp11Rom *const self) {
    WRAP_BODY(
        UnibusDevice,
        UNIBUS_DEVICE_INTERFACE(Pdp11Rom),
        {
            ._reset = pdp11_rom_reset,
            ._try_read = pdp11_rom_try_read,
            ._try_write_word = pdp11_rom_try_write_word,
            ._try_write_byte = pdp11_rom_try_write_byte,
        }
    );
}
