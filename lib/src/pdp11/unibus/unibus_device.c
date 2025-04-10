#include "pdp11/unibus/unibus_device.h"

#include <stddef.h>

#include <assert.h>

static bool no_unibus_device_try_read(
    void *const self,
    uint16_t const addr,
    uint16_t *const out_val
) {
    return false;
}
static bool no_unibus_device_try_read_pause(
    void *const self,
    uint16_t const addr,
    uint16_t *const out_val
) {
    return false;
}
static bool no_unibus_device_try_write_word(
    void *const self,
    uint16_t const addr,
    uint16_t const val
) {
    return false;
}
static bool no_unibus_device_try_write_byte(
    void *const self,
    uint16_t const addr,
    uint8_t const val
) {
    return false;
}

UnibusDevice no_unibus_device(void) {
    void *const self = NULL;
    WRAP_BODY(
        UnibusDevice,
        UNIBUS_DEVICE_INTERFACE(void),
        {
            ._try_read = no_unibus_device_try_read,
            ._try_read_pause = no_unibus_device_try_read_pause,
            ._try_write_word = no_unibus_device_try_write_word,
            ._try_write_byte = no_unibus_device_try_write_byte,
        }
    );
}
