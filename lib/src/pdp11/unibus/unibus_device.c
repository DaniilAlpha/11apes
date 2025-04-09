#include "pdp11/unibus/unibus_device.h"

#include <stddef.h>

#include <assert.h>

static uint16_t *
no_unibus_device_try_address(void *const self, uint16_t const addr) {
    return NULL;
}

UnibusDevice no_unibus_device(void) {
    void *const self = NULL;
    WRAP_BODY(
        UnibusDevice,
        UNIBUS_DEVICE_INTERFACE(void),
        {._try_address = no_unibus_device_try_address}
    );
}
