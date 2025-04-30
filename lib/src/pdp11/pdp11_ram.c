#include "pdp11/pdp11_ram.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <unistd.h>
#include <woodi.h>

Result pdp11_ram_init(
    Pdp11Ram *const self,
    uint16_t const starting_addr,
    uint16_t const size,
    char const *const filepath
) {
    assert(size <= PDP11_RAM_MAX_SIZE);

    void *const data = malloc(size);
    if (!data) return OutOfMemErr;
    self->_data = data;

    self->_starting_addr = starting_addr;
    self->_size = size;
    self->_filepath = filepath;

    if (pdp11_ram_load(self) != Ok) memset((void *)self->_data, 0, self->_size);

    return Ok;
}
void pdp11_ram_uninit(Pdp11Ram *const self) {
    pdp11_ram_save(self);

    free((void *)self->_data), self->_data = NULL;

    self->_starting_addr = self->_size = 0;
}

Result pdp11_ram_save(Pdp11Ram *const self) {
    if (!self->_filepath) return StateErr;

    FILE *const file = fopen(self->_filepath, "w");
    if (!file) return FileUnavailableErr;

    if (fwrite((void *)self->_data, self->_size, 1, file) != 1)
        return fclose(file), FileWritingErr;

    fclose(file);
    return Ok;
}
Result pdp11_ram_load(Pdp11Ram *const self) {
    if (!self->_filepath) return StateErr;

    FILE *const file = fopen(self->_filepath, "r");
    if (!file) return FileUnavailableErr;

    if (fread((void *)self->_data, self->_size, 1, file) != 1)
        return fclose(file), FileReadingErr;

    fclose(file);
    return Ok;
}

/***************
 ** interface **
 ***************/

static void pdp11_ram_reset(Pdp11Ram *const) {}
static bool pdp11_ram_try_read(
    Pdp11Ram *const self,
    uint16_t addr,
    uint16_t *const out_val
) {
    addr -= self->_starting_addr;
    if (!(addr < self->_size)) return false;

    *out_val = *(uint16_t *)(self->_data + addr);

    return true;
}
static bool pdp11_ram_try_write_word(
    Pdp11Ram *const self,
    uint16_t addr,
    uint16_t const val
) {
    addr -= self->_starting_addr;
    if (!(addr < self->_size)) return false;

    *(uint16_t *)(self->_data + addr) = val;

    return true;
}
static bool pdp11_ram_try_write_byte(
    Pdp11Ram *const self,
    uint16_t addr,
    uint8_t const val
) {
    addr -= self->_starting_addr;
    if (!(addr < self->_size)) return false;

    *(uint8_t *)(self->_data + addr) = val;

    return true;
}
UnibusDevice pdp11_ram_ww_unibus_device(Pdp11Ram *const self) {
    WRAP_BODY(
        UnibusDevice,
        UNIBUS_DEVICE_INTERFACE(Pdp11Ram),
        {
            ._reset = pdp11_ram_reset,
            ._try_read = pdp11_ram_try_read,
            ._try_write_word = pdp11_ram_try_write_word,
            ._try_write_byte = pdp11_ram_try_write_byte,
        }
    );
}
