#include "pdp11/unibus/unibus.h"

#include "pdp11/cpu/pdp11_cpu.h"

void unibus_init(
    Unibus *const self,
    Pdp11Cpu *const cpu,
    UnibusLock const sack_lock,
    UnibusLock const bbsy_lock
) {
    self->_sack = sack_lock;
    self->_bbsy = bbsy_lock;
}

// TODO somehow honor horizontal priorities

void unibus_intr(
    Unibus *const self,
    unsigned const priority,
    uint16_t const trap
) {
    if (priority <= self->cpu->stat.priority) return;  // TODO? maybe just wait

    // TODO wait for CPU to finish executing an instruction
    unibus_lock_lock(&self->_sack);
    unibus_lock_lock(&self->_bbsy);
    unibus_lock_unlock(&self->_sack);

    pdp11_cpu_trap(self->cpu, trap);

    unibus_lock_unlock(&self->_bbsy);
}

// TODO! implement properly
uint16_t unibus_dati(Unibus *const self, uint16_t const address) {
    unibus_lock_lock(&self->_sack);
    unibus_lock_lock(&self->_bbsy);
    unibus_lock_unlock(&self->_sack);

    // should wait til device sends SSYN and the data

    unibus_lock_unlock(&self->_bbsy);

    return 0;  // send real data back
}
uint16_t unibus_datip(Unibus *const self, uint16_t const address) {
    return unibus_dati(self, address);
}
void unibus_dato(
    Unibus *const self,
    uint16_t const address,
    uint16_t const data
) {
    unibus_lock_lock(&self->_sack);
    unibus_lock_lock(&self->_bbsy);
    unibus_lock_unlock(&self->_sack);

    // send data to the slave

    // then should wait til device accepts data and sends SSYN

    unibus_lock_unlock(&self->_bbsy);
}
void unibus_datob(
    Unibus *const self,
    uint16_t const address,
    uint8_t const data
) {
    unibus_lock_lock(&self->_sack);
    unibus_lock_lock(&self->_bbsy);
    unibus_lock_unlock(&self->_sack);

    // send data to the slave

    // then should wait til device accepts data and sends SSYN

    unibus_lock_unlock(&self->_bbsy);
}
