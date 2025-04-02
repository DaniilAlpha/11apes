#ifndef UNIBUS_H
#define UNIBUS_H

#include <woodi.h>

// TODO!!! this is all dummy code and not meant to work!!!

#define UNIBUS_DEVICE_INTERFACE(Self)                                          \
  { void (*address)(Self *const self); }

WRAPPER(UnibusDevice, UNIBUS_DEVICE_INTERFACE);

static inline void unibus_device_address(UnibusDevice *const self) {
    return WRAPPER_CALL(address, self);
}
static void cpu_unibus_device_address(int *const self) {}

static UnibusDevice cpu_ww_unibus_device(int *const self) WRAP_BODY(
    UnibusDevice,
    UNIBUS_DEVICE_INTERFACE(int),
    {.address = cpu_unibus_device_address}
);

typedef struct Unibus {
    UnibusDevice *cpu;
    UnibusDevice *master;
    UnibusDevice *slave;
} Unibus;

void unibus_init(Unibus *const self, UnibusDevice *const cpu) {
    self->cpu = cpu;
}

#endif
