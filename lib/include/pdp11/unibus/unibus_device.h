#ifndef UNIBUS_DEVICE_H
#define UNIBUS_DEVICE_H

#include <stdint.h>

#include <woodi.h>

#define UNIBUS_DEVICE_INTERFACE(Self)                                          \
  { uint16_t *(*const _try_address)(Self *const self, uint16_t const addr); }
WRAPPER(UnibusDevice, UNIBUS_DEVICE_INTERFACE);

UnibusDevice no_unibus_device(void);

#endif
