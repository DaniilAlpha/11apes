#include "pdp11/unibus/unibus.h"

#include <stddef.h>

#include "conviniences.h"
#include "pdp11/cpu/pdp11_cpu.h"

/*************
 ** private **
 *************/

static inline uint16_t *
unibus_device_try_address(UnibusDevice const *const self, uint16_t const addr) {
    return WRAPPER_CALL(_try_address, self, addr);
}
static uint16_t *
unibus_try_address(Unibus const *const self, uint16_t const addr) {
    foreach (device_ptr, self->devices) {
        uint16_t *const data_ptr = unibus_device_try_address(device_ptr, addr);
        if (data_ptr) return data_ptr;
    }
    return NULL;
}

/************
 ** public **
 ************/

void unibus_init(
    Unibus *const self,
    Pdp11Cpu *const cpu,
    UnibusLock const sack_lock,
    UnibusLock const bbsy_lock
) {
    self->_sack = sack_lock;
    self->_bbsy = bbsy_lock;

    foreach (device_ptr, self->devices) *device_ptr = no_unibus_device();

    self->cpu = cpu;
}

void unibus_intr(
    Unibus *const self,
    unsigned const priority,
    uint16_t const trap
) {
    // TODO somehow honor horizontal priorities
    if (priority <= self->cpu->stat.priority) return;  // TODO? maybe just wait

    // TODO wait for CPU to finish executing an instruction
    unibus_lock_lock(&self->_sack);
    unibus_lock_lock(&self->_bbsy);
    unibus_lock_unlock(&self->_sack);

    pdp11_cpu_trap(self->cpu, trap);

    unibus_lock_unlock(&self->_bbsy);
}

uint16_t unibus_dati(Unibus *const self, uint16_t const addr) {
    unibus_lock_lock(&self->_sack);
    unibus_lock_lock(&self->_bbsy);
    unibus_lock_unlock(&self->_sack);

    uint16_t const *const data_ptr = unibus_try_address(self, addr);
    // TODO some error should be here
    if (!data_ptr) return unibus_lock_unlock(&self->_bbsy), 0111111;

    unibus_lock_unlock(&self->_bbsy);

    return *data_ptr;
}
uint16_t unibus_datip(Unibus *const self, uint16_t const addr) {
    return unibus_dati(self, addr);
}
void unibus_dato(Unibus *const self, uint16_t const addr, uint16_t const data) {
    unibus_lock_lock(&self->_sack);
    unibus_lock_lock(&self->_bbsy);
    unibus_lock_unlock(&self->_sack);

    uint16_t *const data_ptr = unibus_try_address(self, addr);
    // TODO some error should be here
    if (!data_ptr) return unibus_lock_unlock(&self->_bbsy);

    *data_ptr = data;

    unibus_lock_unlock(&self->_bbsy);
}
void unibus_datob(Unibus *const self, uint16_t const addr, uint8_t const data) {
    unibus_lock_lock(&self->_sack);
    unibus_lock_lock(&self->_bbsy);
    unibus_lock_unlock(&self->_sack);

    // TODO! implement properly - currently not aligned, not checked, also, may
    // be a different location
    uint8_t *const data_ptr = (uint8_t *)unibus_try_address(self, addr);
    // TODO some error should be here
    if (!data_ptr) return unibus_lock_unlock(&self->_bbsy);

    *data_ptr = data;

    unibus_lock_unlock(&self->_bbsy);
}
