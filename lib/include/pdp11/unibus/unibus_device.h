#ifndef UNIBUS_DEVICE_H
#define UNIBUS_DEVICE_H

#include <stdbool.h>
#include <stdint.h>

#include <woodi.h>

#define UNIBUS_DEVICE_INTERFACE(Self)                                          \
  {                                                                            \
    bool (*const _try_read                                                     \
    )(Self *const self, uint16_t const addr, uint16_t *const out_val);         \
    bool (*const _try_read_pause                                               \
    )(Self *const self, uint16_t const addr, uint16_t *const out_val);         \
    bool (*const _try_write_word                                               \
    )(Self *const self, uint16_t const addr, uint16_t const val);              \
    bool (*const _try_write_byte                                               \
    )(Self *const self, uint16_t const addr, uint8_t const val);               \
  }
WRAPPER(UnibusDevice, UNIBUS_DEVICE_INTERFACE);

UnibusDevice no_unibus_device(void);

#endif
