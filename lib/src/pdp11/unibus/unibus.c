#include "pdp11/unibus/unibus.h"

#include <stddef.h>
#include <stdio.h>

#include <unistd.h>

#include "conviniences.h"
#include "pdp11/cpu/pdp11_cpu.h"

/*************
 ** private **
 *************/

static inline void unibus_device_reset(UnibusDevice const *const self) {
    return WRAPPER_CALL(_reset, self);
}
static inline bool unibus_device_try_read(
    UnibusDevice const *const self,
    uint16_t const addr,
    uint16_t *const out
) {
    return WRAPPER_CALL(_try_read, self, addr, out);
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
    uint16_t *const out
) {
    if (addr == UNIBUS_CPU_PSW_ADDRESS)
        return *out = pdp11_psw_to_word(pdp11_cpu_psw(self->_cpu)), true;

    foreach (device_ptr, self->devices, self->devices + UNIBUS_DEVICE_COUNT)
        if (unibus_device_try_read(device_ptr, addr, out)) return true;
    return false;
}
static bool unibus_try_write_word(
    Unibus const *const self,
    uint16_t const addr,
    uint16_t const val
) {
    if (addr == UNIBUS_CPU_PSW_ADDRESS)
        return pdp11_cpu_psw(self->_cpu) = pdp11_psw(val), true;

    foreach (device_ptr, self->devices, self->devices + UNIBUS_DEVICE_COUNT)
        if (unibus_device_try_write_word(device_ptr, addr, val)) return true;
    return false;
}
static bool unibus_try_write_byte(
    Unibus const *const self,
    uint16_t const addr,
    uint8_t const val
) {
    if (addr == UNIBUS_CPU_PSW_ADDRESS)
        return pdp11_cpu_psw(self->_cpu) = pdp11_psw(
                   (pdp11_psw_to_word(pdp11_cpu_psw(self->_cpu)) & 0xFF00) | val
               ),
               true;

    foreach (device_ptr, self->devices, self->devices + UNIBUS_DEVICE_COUNT)
        if (unibus_device_try_write_byte(device_ptr, addr, val)) return true;
    return false;
}

/* Assumes SACK is locked. Unlocks SACK. Locks BBSY. */
static void unibus_switch_to_next_master(Unibus *const self) {
    pthread_mutex_lock(&self->_bbsy);
    self->_master = self->_next_master;
    self->_next_master = UNIBUS_DEVICE_CPU;
    pthread_mutex_unlock(&self->_sack);
}
/* Assumes BBSY is locked. Unlocks BSSY. */
static void unibus_drop_current_master(Unibus *const self) {
    self->_master = UNIBUS_DEVICE_CPU;
    pthread_mutex_unlock(&self->_bbsy);
}

/* Locks BBSY. */
static void unibus_switch_to_cpu_master(Unibus *const self) {
    pthread_mutex_lock(&self->_bbsy);
    assert(self->_master == UNIBUS_DEVICE_CPU);
}
/* Assumes BBSY is locked. Unlocks BSSY. */
static void unibus_drop_cpu_master(Unibus *const self) {
    pthread_mutex_unlock(&self->_bbsy);
}

/************
 ** public **
 ************/

void unibus_init(Unibus *const self, Pdp11Cpu *const cpu) {
    pthread_mutex_init(&self->_sack, NULL);
    pthread_mutex_init(&self->_bbsy, NULL);

    foreach (device_ptr, self->devices, self->devices + UNIBUS_DEVICE_COUNT)
        *device_ptr = no_unibus_device();
    self->_cpu = cpu;

    self->_master = self->_next_master = UNIBUS_DEVICE_CPU;
}
void unibus_uninit(Unibus *const self) {
    pthread_mutex_destroy(&self->_sack);
    pthread_mutex_destroy(&self->_bbsy);
}
void unibus_reset(Unibus *const self) {
    pdp11_cpu_reset(self->_cpu);
    foreach (device_ptr, self->devices, self->devices + UNIBUS_DEVICE_COUNT)
        unibus_device_reset(device_ptr);
}

void unibus_br_intr(
    Unibus *const self,
    unsigned const priority,
    UnibusDevice const *const device,
    uint8_t const intr
) {
    // br
    pthread_mutex_lock(&self->_sack);
    // TODO? maybe replace with cond var
    while (priority <= ((Pdp11Psw volatile)pdp11_cpu_psw(self->_cpu)).priority
    ) {
        pthread_mutex_unlock(&self->_sack);
        sleep(0);
        pthread_mutex_lock(&self->_sack);
    }
    self->_next_master = device;
    // intr
    unibus_switch_to_next_master(self);
    self->_master = UNIBUS_DEVICE_CPU;
    pdp11_cpu_intr(self->_cpu, intr);
    unibus_drop_current_master(self);
}

Result unibus_npr_dati(
    Unibus *const self,
    UnibusDevice const *const device,
    uint16_t const addr,
    uint16_t *const out
) {
    // npr
    pthread_mutex_lock(&self->_sack);
    self->_next_master = device;
    // dati
    unibus_switch_to_next_master(self);
    if ((addr & 1) == 1 || !unibus_try_read(self, addr, out))
        return unibus_drop_current_master(self), UnknownErr;
    unibus_drop_current_master(self);
    return Ok;
}
Result unibus_npr_dato(
    Unibus *const self,
    UnibusDevice const *const device,
    uint16_t const addr,
    uint16_t const data
) {
    // npr
    pthread_mutex_lock(&self->_sack);
    self->_next_master = device;
    // dato
    unibus_switch_to_next_master(self);
    if ((addr & 1) == 1 || !unibus_try_write_word(self, addr, data))
        return unibus_drop_current_master(self), UnknownErr;
    unibus_drop_current_master(self);
    return Ok;
}
Result unibus_npr_datob(
    Unibus *const self,
    UnibusDevice const *const device,
    uint16_t const addr,
    uint8_t const data
) {
    // npr
    pthread_mutex_lock(&self->_sack);
    self->_next_master = device;
    // dato
    unibus_switch_to_next_master(self);
    if (!unibus_try_write_byte(self, addr, data))
        return unibus_drop_current_master(self), UnknownErr;
    unibus_drop_current_master(self);
    return Ok;
}

Result
unibus_cpu_dati(Unibus *const self, uint16_t const addr, uint16_t *const out) {
    unibus_switch_to_cpu_master(self);
    if ((addr & 1) == 1 || !unibus_try_read(self, addr, out))
        return unibus_drop_cpu_master(self), UnknownErr;
    unibus_drop_cpu_master(self);
    return Ok;
}
Result
unibus_cpu_dato(Unibus *const self, uint16_t const addr, uint16_t const data) {
    unibus_switch_to_cpu_master(self);
    if ((addr & 1) == 1 || !unibus_try_write_word(self, addr, data))
        return unibus_drop_cpu_master(self), UnknownErr;
    unibus_drop_cpu_master(self);
    return Ok;
}
Result
unibus_cpu_datob(Unibus *const self, uint16_t const addr, uint8_t const data) {
    unibus_switch_to_cpu_master(self);
    if (!unibus_try_write_byte(self, addr, data))
        return unibus_drop_cpu_master(self), UnknownErr;
    unibus_drop_cpu_master(self);
    return Ok;
}
