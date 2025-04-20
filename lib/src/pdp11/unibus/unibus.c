#include "pdp11/unibus/unibus.h"

#include <stddef.h>

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
    if (self->_master != device) {
        // TODO replace with cond var
        pthread_mutex_lock(&self->_bbsy);
        while (self->_next_master != device) {
            // TODO wait for cpu prior changed
            pthread_mutex_unlock(&self->_bbsy);
            usleep(0);
            pthread_mutex_lock(&self->_bbsy);
        }

        self->_master = self->_next_master;
        self->_next_master = UNIBUS_DEVICE_CPU;
    }
}
static void unibus_drop_current_master(Unibus *const self) {
    self->_master = UNIBUS_DEVICE_CPU;
    pthread_mutex_unlock(&self->_bbsy);
}

/* Assumes BBSY is locked. Transfers control to CPU and services the interrupt.
 */
static void
unibus_transfer_control_to_cpu_intr(Unibus *const self, uint8_t const intr) {
    self->_master = UNIBUS_DEVICE_CPU;
    // TODO!!! should not be INTR in case of unibus error
    pdp11_cpu_intr(self->_cpu, intr);
}

static uint16_t unibus_dati_helper(
    Unibus *const self,
    UnibusDevice const *const device,
    uint16_t const addr
) {
    unibus_become_master(self, device);

    uint16_t data = 0xDEC;
    if ((addr & 1) == 1 || !unibus_try_read(self, addr, &data))
        unibus_transfer_control_to_cpu_intr(self, PDP11_CPU_TRAP_UNIBUS_ERR);

    unibus_drop_current_master(self);
    return data;
}
static void unibus_dato_helper(
    Unibus *const self,
    UnibusDevice const *const device,
    uint16_t const addr,
    uint16_t const data
) {
    unibus_become_master(self, device);

    if ((addr & 1) == 1 || !unibus_try_write_word(self, addr, data))
        unibus_transfer_control_to_cpu_intr(self, PDP11_CPU_TRAP_UNIBUS_ERR);

    unibus_drop_current_master(self);
}
static void unibus_datob_helper(
    Unibus *const self,
    UnibusDevice const *const device,
    uint16_t const addr,
    uint8_t const data
) {
    unibus_become_master(self, device);

    if (!unibus_try_write_byte(self, addr, data))
        unibus_transfer_control_to_cpu_intr(self, PDP11_CPU_TRAP_UNIBUS_ERR);

    unibus_drop_current_master(self);
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

/* Waits til CPU priority is sufficient, waits and locks SACK, then makes device
 * next master. Other devices are unable to BR or NPR until unlocked again.
 * Switches to the next master (intermediate), transfers to CPU to service the
 * interrupt, unlocking BBSY after. */
void unibus_br_intr(
    Unibus *const self,
    unsigned const priority,
    UnibusDevice const *const device,
    uint8_t const intr
) {
    // br

    pthread_mutex_lock(&self->_sack);
    while (priority <=
           ((Pdp11CpuStat volatile)pdp11_cpu_stat(self->_cpu)).priority) {
        // TODO wait for cpu prior changed
        pthread_mutex_unlock(&self->_sack);
        usleep(0);
        pthread_mutex_lock(&self->_sack);
    }

    self->_next_master = device;

    // intr

    unibus_become_master(self, UNIBUS_DEVICE_CPU);
    unibus_transfer_control_to_cpu_intr(self, intr);
    unibus_drop_current_master(self);
    // TODO!!!? maybe too long for a sack
    pthread_mutex_unlock(&self->_sack);
}

uint16_t unibus_npr_dati(
    Unibus *const self,
    UnibusDevice const *const device,
    uint16_t const addr
) {
    pthread_mutex_lock(&self->_sack);
    self->_next_master = device;
    uint16_t const val = unibus_dati_helper(self, device, addr);
    // TODO!!!? maybe too long for a sack
    pthread_mutex_unlock(&self->_sack);
    return val;
}
void unibus_npr_dato(
    Unibus *const self,
    UnibusDevice const *const device,
    uint16_t const addr,
    uint16_t const data
) {
    pthread_mutex_lock(&self->_sack);
    self->_next_master = device;
    unibus_dato_helper(self, device, addr, data);
    // TODO!!!? maybe too long for a sack
    pthread_mutex_unlock(&self->_sack);
}
void unibus_npr_datob(
    Unibus *const self,
    UnibusDevice const *const device,
    uint16_t const addr,
    uint8_t const data
) {
    pthread_mutex_lock(&self->_sack);
    self->_next_master = device;
    unibus_datob_helper(self, device, addr, data);
    // TODO!!!? maybe too long for a sack
    pthread_mutex_unlock(&self->_sack);
}

uint16_t unibus_cpu_dati(Unibus *const self, uint16_t const addr) {
    uint16_t const value = unibus_dati_helper(self, UNIBUS_DEVICE_CPU, addr);
    // TODO!!!? is even needed?
    pthread_mutex_unlock(&self->_sack);
    return value;
}
void unibus_cpu_dato(
    Unibus *const self,
    uint16_t const addr,
    uint16_t const data
) {
    unibus_dato_helper(self, UNIBUS_DEVICE_CPU, addr, data);
    // TODO!!!? is even needed?
    pthread_mutex_unlock(&self->_sack);
}
void unibus_cpu_datob(
    Unibus *const self,
    uint16_t const addr,
    uint8_t const data
) {
    unibus_datob_helper(self, UNIBUS_DEVICE_CPU, addr, data);
    // TODO!!!? is even needed?
    pthread_mutex_unlock(&self->_sack);
}
