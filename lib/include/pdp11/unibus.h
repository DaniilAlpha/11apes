#ifndef UNIBUS_H
#define UNIBUS_H

#include <stdint.h>

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
    UnibusDevice *master;
    UnibusDevice *slave;
} Unibus;

void unibus_init(Unibus *const self);

void unibus_become_master(uint16_t const slave_address);

void unibus_br(
    Unibus *const self,
    unsigned const priority
);  // TODO request bus for a CPU interrupt: hang until bus can process
// interrupt (and current instr should be finished). Not really mean that bus
// control is given to another device, but just places it next in a queue. Other
// BRs and NPRs cannot be processed until bus is available and given to the
// device. [Then, should set BBSY signal (and drop SACK), to capture a master.
// Not sure if it has to be another function]. Should somehow send an interrupt
// vector address to the CPU, then wait til CPU sends SSYN and only then to end
// and release

void unibus_npr(Unibus *const self
);  // TODO request bus for a data transaction: hang until bus can process
    // interrupt (can happen mid-cycle). Should somehow send slave address,
    // transaction type (
    //  DATO/* data out of master */,
    //  DATOB /* byte */,
    //  DATI /* into the master */,
    //  DATIP /* for old sucky core mem, you will have it the same as DATI */
    // ) and data (for DATO and DATOB), then wait til device accepts data and
    // sends SSYN and sends data (for DATI and DATIP), then end and release.

void unibus_release(Unibus *const self);  // drop BBSY

#endif
