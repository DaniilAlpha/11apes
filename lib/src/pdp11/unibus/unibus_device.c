#include "pdp11/unibus/unibus_device.h"

#include <stddef.h>

#include <assert.h>

static void no_unibus_device_reset(void *const) {}
static bool
no_unibus_device_try_read(void *const, uint16_t const, uint16_t *const) {
    return false;
}
static bool
no_unibus_device_try_write_word(void *const, uint16_t const, uint16_t const) {
    return false;
}
static bool
no_unibus_device_try_write_byte(void *const, uint16_t const, uint8_t const) {
    return false;
}

UnibusDevice no_unibus_device(void) {
    void *const self = NULL;
    WRAP_BODY(
        UnibusDevice,
        UNIBUS_DEVICE_INTERFACE(void),
        {
            ._reset = no_unibus_device_reset,
            ._try_read = no_unibus_device_try_read,
            ._try_write_word = no_unibus_device_try_write_word,
            ._try_write_byte = no_unibus_device_try_write_byte,
        }
    );
}
