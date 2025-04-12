#ifndef UNIBUS_H
#define UNIBUS_H

#include <stdint.h>

#include <woodi.h>

#include "pdp11/unibus/unibus_device.h"
#include "pdp11/unibus/unibus_lock.h"

#define UNIBUS_DEVICE_COUNT (8)

#define UNIBUS_CPU_REG_ADDRESS  (0177700)
#define UNIBUS_CPU_STAT_ADDRESS (0177777)

typedef struct Pdp11Cpu Pdp11Cpu;
typedef struct Unibus {
    UnibusLock _bbsy,
        _sack;  // TODO? sack seems completely redundant, but
                // emu may behave slightly different without it. should consider
                // its removal later

    Pdp11Cpu *_cpu;
    UnibusDevice devices[UNIBUS_DEVICE_COUNT];
} Unibus;

void unibus_init(
    Unibus *const self,
    Pdp11Cpu *const cpu,
    UnibusLock const sack_lock,
    UnibusLock const bbsy_lock
);

void unibus_br(
    Unibus *const self,
    unsigned const priority,
    uint16_t const trap
);

uint16_t unibus_dati(Unibus *const self, uint16_t const addr);
// TODO? DATIP should be used when retreiving destination value in instructions,
// but maybe there is not difference
uint16_t unibus_datip(Unibus *const self, uint16_t const addr);
void unibus_dato(Unibus *const self, uint16_t const addr, uint16_t const data);
void unibus_datob(Unibus *const self, uint16_t const addr, uint8_t const data);

#endif
