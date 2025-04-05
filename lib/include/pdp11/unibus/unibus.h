#ifndef UNIBUS_H
#define UNIBUS_H

#include <stdint.h>

#include <woodi.h>

#include "pdp11/cpu/pdp11_cpu.h"
#include "pdp11/unibus/unibus_lock.h"

#define UNIBUS_DEVICE_INTERFACE(Self)                                          \
    { void (*address)(Self *const self); }
WRAPPER(UnibusDevice, UNIBUS_DEVICE_INTERFACE);

typedef struct Unibus {
    UnibusLock _bbsy,
        _sack;  // TODO? sack seems completely redundant, but
                // emu may behave slightly different without it. should consider
                // its removal later

    Pdp11Cpu *cpu;
} Unibus;

void unibus_init(
    Unibus *const self,
    Pdp11Cpu *const cpu,
    UnibusLock const sack_lock,
    UnibusLock const bbsy_lock
);

void unibus_intr(
    Unibus *const self,
    unsigned const priority,
    uint16_t const trap
);

uint16_t unibus_dati(Unibus *const self, uint16_t const address);
uint16_t unibus_datip(Unibus *const self, uint16_t const address);
void unibus_dato(
    Unibus *const self,
    uint16_t const address,
    uint16_t const data
);
void unibus_datob(
    Unibus *const self,
    uint16_t const address,
    uint8_t const data
);

#endif
