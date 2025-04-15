#include "pdp11/pdp11_rom.h"

#include <stdlib.h>

#include <assert.h>

#include "woodi.h"

static long fsize(FILE *const self) {
    long const cur = ftell(self);
    fseek(self, 0, SEEK_END);
    long const size = ftell(self);
    fseek(self, cur, SEEK_CUR);
    return size;
}

static bool pdp11_rom_try_read(
    Pdp11Rom *const self,
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
static bool pdp11_rom_try_write_word(
    Pdp11Rom *const self,
    uint16_t const addr,
    uint16_t const _
) {
    return self->_starting_addr <= addr &&
           addr < self->_starting_addr + self->_size;
}
static bool pdp11_rom_try_write_byte(
    Pdp11Rom *const self,
    uint16_t const addr,
    uint8_t const _
) {
    return self->_starting_addr <= addr &&
           addr < self->_starting_addr + self->_size;
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
    if (fread(data, 1, size, file) != size) return FileReadingErr;
    self->_data = data;

    self->_starting_addr = starting_addr;
    self->_size = size;

    return Ok;
}
void pdp11_rom_uninit(Pdp11Rom *const self) {
    free((void *)self->_data), self->_data = NULL;

    self->_starting_addr = self->_size = 0;
}

UnibusDevice pdp11_rom_ww_unibus_device(Pdp11Rom *const self) {
    WRAP_BODY(
        UnibusDevice,
        UNIBUS_DEVICE_INTERFACE(Pdp11Rom),
        {
            ._try_read = pdp11_rom_try_read,
            ._try_write_word = pdp11_rom_try_write_word,
            ._try_write_byte = pdp11_rom_try_write_byte,
        }
    );
}
