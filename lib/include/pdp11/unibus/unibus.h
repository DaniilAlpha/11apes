#ifndef UNIBUS_H
#define UNIBUS_H

#include <stddef.h>
#include <stdint.h>

#include <woodi.h>

#include "pdp11/unibus/unibus_device.h"
#include "pdp11/unibus/unibus_lock.h"

#define UNIBUS_DEVICE_COUNT (8)

#define UNIBUS_CPU_REG_ADDRESS  (0177700)
#define UNIBUS_CPU_STAT_ADDRESS (0177776)

#define UNIBUS_DEVICE_CPU (NULL)

typedef struct Pdp11Cpu Pdp11Cpu;
typedef struct Unibus {
    UnibusDevice devices[UNIBUS_DEVICE_COUNT];

    UnibusLock _bbsy, _sack;
    UnibusDevice const *_current_master, *_next_master;

    Pdp11Cpu *_cpu;
} Unibus;

void unibus_init(
    Unibus *const self,
    Pdp11Cpu *const cpu,
    UnibusLock const sack_lock,
    UnibusLock const bbsy_lock
);

static inline bool unibus_is_running(Unibus const *const self) {
    return self->_current_master == NULL;
}
static inline bool unibus_is_periph_master(Unibus const *const self) {
    return self->_current_master != NULL;
}

void unibus_reset(Unibus *const self);

void unibus_br(
    Unibus *const self,
    UnibusDevice const *const device,
    unsigned const priority
);
void unibus_npr(Unibus *const self, UnibusDevice const *const device);

void unibus_intr(
    Unibus *const self,
    UnibusDevice const *const device,
    uint8_t const intr
);
uint16_t unibus_dati(
    Unibus *const self,
    UnibusDevice const *const device,
    uint16_t const addr
);
void unibus_dato(
    Unibus *const self,
    UnibusDevice const *const device,
    uint16_t const addr,
    uint16_t const data
);
void unibus_datob(
    Unibus *const self,
    UnibusDevice const *const device,
    uint16_t const addr,
    uint8_t const data
);

#endif
