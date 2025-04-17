#include "pdp11/unibus/unibus.h"

#include <stddef.h>

#include <unistd.h>

#include "conviniences.h"
#include "pdp11/cpu/pdp11_cpu.h"

// TODO!! continue redesigning unibus with simething like `become_master` or
// `enslave`, keep NPRs and interrutpt will automatically make CPU a master. The
// best way is to return something like `UnibusChannel` that is one-time
// channel, maybe even sepatate for Brs and Nprs.

/*************
 ** private **
 *************/

static inline void unibus_device_reset(UnibusDevice const *const self) {
    return WRAPPER_CALL(_reset, self);
}

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
        *out_val = pdp11_cpu_stat_to_word(&pdp11_cpu_stat(self->_cpu));
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
        pdp11_cpu_stat(self->_cpu) = pdp11_cpu_stat_from_word(val);
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
    case UNIBUS_CPU_STAT_ADDRESS: {
        pdp11_cpu_stat(self->_cpu) = pdp11_cpu_stat_from_word(
            (pdp11_cpu_stat_to_word(&pdp11_cpu_stat(self->_cpu)) & 0xFF00) | val
        );
    }
        return true;
    }

    foreach (device_ptr, self->devices, self->devices + UNIBUS_DEVICE_COUNT)
        if (unibus_device_try_write_byte(device_ptr, addr, val)) return true;
    return false;
}

static void
unibus_become_master(Unibus *const self, UnibusDevice const *const device) {
    assert(self->_next_master == device);

    unibus_lock_lock(&self->_bbsy);
    self->_current_master = self->_next_master;
    unibus_lock_unlock(&self->_sack);
}
static void unibus_trap_to_error(Unibus *const self) {
    self->_current_master = self->_next_master = UNIBUS_DEVICE_CPU;
    pdp11_cpu_trap(self->_cpu, PDP11_CPU_TRAP_UNIBUS_ERR);

    unibus_lock_unlock(&self->_bbsy);
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

    self->_current_master = self->_next_master = UNIBUS_DEVICE_CPU;
}

void unibus_reset(Unibus *const self) {
    pdp11_cpu_reset(self->_cpu);
    foreach (device_ptr, self->devices, self->devices + UNIBUS_DEVICE_COUNT)
        unibus_device_reset(device_ptr);
}

void unibus_br(
    Unibus *const self,
    UnibusDevice const *const device,
    unsigned const priority
) {
    unibus_lock_lock(&self->_sack);
    while (priority <=
           ((Pdp11CpuStat volatile)pdp11_cpu_stat(self->_cpu)).priority) {
        // TODO wait for cpu prior changed
        unibus_lock_unlock(&self->_sack);
        usleep(0);
        unibus_lock_lock(&self->_sack);
    }

    // TODO wait for CPU to finish executing an instruction (!when
    // instruction causes trap, should wait one more: handbook p. 65!)

    self->_next_master = device;
}
void unibus_npr(Unibus *const self, UnibusDevice const *const device) {
    unibus_lock_lock(&self->_sack);
    self->_next_master = device;
}
void unibus_intr(
    Unibus *const self,
    UnibusDevice const *const device,
    uint8_t const intr
) {
    if (self->_current_master != device) unibus_become_master(self, device);

    self->_current_master = self->_next_master = UNIBUS_DEVICE_CPU;
    pdp11_cpu_trap(self->_cpu, intr);
    unibus_lock_unlock(&self->_bbsy);
}

uint16_t unibus_dati(
    Unibus *const self,
    UnibusDevice const *const device,
    uint16_t const addr
) {
    if (self->_current_master != device) unibus_become_master(self, device);

    uint16_t data = 0xDEC;

    bool const was_device_addressed = unibus_try_read(self, addr, &data);
    if ((addr & 1) == 1 || !was_device_addressed)
        return unibus_trap_to_error(self), data;

    self->_current_master = self->_next_master;
    self->_next_master = UNIBUS_DEVICE_CPU;
    unibus_lock_unlock(&self->_bbsy);

    return data;
}
void unibus_dato(
    Unibus *const self,
    UnibusDevice const *const device,
    uint16_t const addr,
    uint16_t const data
) {
    if (self->_current_master != device) unibus_become_master(self, device);

    bool const was_device_addressed = unibus_try_write_word(self, addr, data);
    if ((addr & 1) == 1 || !was_device_addressed)
        return unibus_trap_to_error(self);

    self->_current_master = self->_next_master,
    self->_next_master = UNIBUS_DEVICE_CPU;
    unibus_lock_unlock(&self->_bbsy);
}
void unibus_datob(
    Unibus *const self,
    UnibusDevice const *const device,
    uint16_t const addr,
    uint8_t const data
) {
    if (self->_current_master != device) unibus_become_master(self, device);

    assert(self->_current_master == device);

    bool const was_device_addressed = unibus_try_write_byte(self, addr, data);
    if (!was_device_addressed) return unibus_trap_to_error(self);

    self->_current_master = self->_next_master;
    self->_next_master = UNIBUS_DEVICE_CPU;
    unibus_lock_unlock(&self->_bbsy);
}
