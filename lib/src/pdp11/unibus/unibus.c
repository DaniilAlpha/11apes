#include "pdp11/unibus/unibus.h"

#include <stddef.h>

#include "conviniences.h"
#include "pdp11/cpu/pdp11_cpu.h"

/*************
 ** private **
 *************/

static inline bool unibus_device_try_read(
    UnibusDevice const *const self,
    uint16_t const addr,
    uint16_t *const out_val
) {
    return WRAPPER_CALL(_try_read, self, addr, out_val);
}
static inline bool unibus_device_try_write_word(
    UnibusDevice const *const self,
    uint16_t const addr,
    uint16_t const val

) {
    return WRAPPER_CALL(_try_write_word, self, addr, val);
}
static inline bool unibus_device_try_write_byte(
    UnibusDevice const *const self,
    uint16_t const addr,
    uint8_t const val

) {
    return WRAPPER_CALL(_try_write_byte, self, addr, val);
}
static bool unibus_try_read(
    Unibus const *const self,
    uint16_t const addr,
    uint16_t *const out_val
) {
    switch (addr) {
    case UNIBUS_CPU_REG_ADDRESS ... UNIBUS_CPU_REG_ADDRESS + 7 * 2:
        *out_val = pdp11_cpu_rx(self->_cpu, UNIBUS_CPU_REG_ADDRESS - addr);
        return true;
    case UNIBUS_CPU_STAT_ADDRESS:
        *out_val = pdp11_cpu_stat_to_word(&self->_cpu->stat);
        return true;
    }

    foreach (device_ptr, self->devices, self->devices + UNIBUS_DEVICE_COUNT)
        if (unibus_device_try_read(device_ptr, addr, out_val)) return true;
    return false;
}
static bool unibus_try_write_word(
    Unibus const *const self,
    uint16_t const addr,
    uint16_t const val
) {
    switch (addr) {
    case UNIBUS_CPU_REG_ADDRESS ... UNIBUS_CPU_REG_ADDRESS + 7 * 2:
        pdp11_cpu_rx(self->_cpu, UNIBUS_CPU_REG_ADDRESS - addr) = val;
        return true;
    case UNIBUS_CPU_STAT_ADDRESS:
        self->_cpu->stat = pdp11_cpu_stat(val);
        return true;
    }

    foreach (device_ptr, self->devices, self->devices + UNIBUS_DEVICE_COUNT)
        if (unibus_device_try_write_word(device_ptr, addr, val)) return true;
    return false;
}
static bool unibus_try_write_byte(
    Unibus const *const self,
    uint16_t const addr,
    uint8_t const val
) {
    switch (addr) {
    case UNIBUS_CPU_REG_ADDRESS ... UNIBUS_CPU_REG_ADDRESS + 7 * 2:
        pdp11_cpu_rl(self->_cpu, UNIBUS_CPU_REG_ADDRESS - addr) = val;
        return true;
    case UNIBUS_CPU_STAT_ADDRESS:
        self->_cpu->stat = pdp11_cpu_stat(
            (pdp11_cpu_stat_to_word(&self->_cpu->stat) & 0xFF00) | val
        );
        return true;
    }

    foreach (device_ptr, self->devices, self->devices + UNIBUS_DEVICE_COUNT)
        if (unibus_device_try_write_byte(device_ptr, addr, val)) return true;
    return false;
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

    foreach (device_ptr, self->devices, self->devices + UNIBUS_DEVICE_COUNT)
        *device_ptr = no_unibus_device();
    self->_cpu = cpu;
}

void unibus_br(
    Unibus *const self,
    unsigned const priority,
    uint16_t const trap
) {
    // TODO somehow honor horizontal priorities
    // TODO this is bad, should be refactored in some future
    while (priority <= ((Pdp11CpuStat volatile)self->_cpu->stat).priority);

    // TODO wait for CPU to finish executing an instruction (!when instruction
    // causes trap, should wait one more: handbook p. 65!)
    unibus_lock_lock(&self->_sack);
    unibus_lock_lock(&self->_bbsy);
    unibus_lock_unlock(&self->_sack);

    pdp11_cpu_trap(self->_cpu, trap);

    unibus_lock_unlock(&self->_bbsy);
}

uint16_t unibus_dati(Unibus *const self, uint16_t const addr) {
    unibus_lock_lock(&self->_sack);
    unibus_lock_lock(&self->_bbsy);
    unibus_lock_unlock(&self->_sack);

    uint16_t data = 0111111;
    if (!unibus_try_read(self, addr, &data)) {
        // TODO some error should be here
    }

    unibus_lock_unlock(&self->_bbsy);

    return data;
}
void unibus_dato(Unibus *const self, uint16_t const addr, uint16_t const data) {
    // TODO trap CPU_ERR if odd address

    unibus_lock_lock(&self->_sack);
    unibus_lock_lock(&self->_bbsy);
    unibus_lock_unlock(&self->_sack);

    if (!unibus_try_write_word(self, addr, data)) {
        // TODO some error should be here
    }

    unibus_lock_unlock(&self->_bbsy);
}
void unibus_datob(Unibus *const self, uint16_t const addr, uint8_t const data) {
    unibus_lock_lock(&self->_sack);
    unibus_lock_lock(&self->_bbsy);
    unibus_lock_unlock(&self->_sack);

    if (!unibus_try_write_byte(self, addr, data)) {
        // TODO some error should be here
    }

    unibus_lock_unlock(&self->_bbsy);
}
