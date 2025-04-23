#include "pdp11/pdp11_console.h"

#include <stdlib.h>

#include "conviniences.h"

static bool pdp11_console_simulated_light(Pdp11Console const *const self) {
    return pdp11_cpu_state(&self->_pdp11->cpu) == PDP11_CPU_STATE_RUN &&
           rand() & 1;
}

void pdp11_console_init(Pdp11Console *const self, Pdp11 *const pdp11) {
    self->_pdp11 = pdp11;

    self->_switch_register = 0;
    self->_addr_register = self->_data_register = 0;

    self->_enable_switch = true;
    self->_power_control_switch = PDP11_CONSOLE_POWER_CONTROL_OFF;

    self->_is_deposit_pressed_consecutively = false;
    self->_is_examine_pressed_consecutively = false;
}

uint16_t pdp11_console_address_indicator(Pdp11Console const *const self) {
    if (self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_OFF)
        return 0;

    return self->_addr_register;
}
uint16_t pdp11_console_data_indicator(Pdp11Console const *const self) {
    if (self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_OFF)
        return 0;

    return self->_data_register;
}

void pdp11_console_next_power_control(Pdp11Console *const self) {
    switch (self->_power_control_switch) {
    case PDP11_CONSOLE_POWER_CONTROL_OFF:
        unibus_reset(&self->_pdp11->unibus);
        self->_power_control_switch = PDP11_CONSOLE_POWER_CONTROL_POWER;
        break;
    case PDP11_CONSOLE_POWER_CONTROL_POWER:
        self->_power_control_switch = PDP11_CONSOLE_POWER_CONTROL_LOCK;
        break;
    case PDP11_CONSOLE_POWER_CONTROL_LOCK: break;
    }
}
void pdp11_console_prev_power_control(Pdp11Console *const self) {
    switch (self->_power_control_switch) {
    case PDP11_CONSOLE_POWER_CONTROL_OFF: break;
    case PDP11_CONSOLE_POWER_CONTROL_POWER:
        self->_power_control_switch = PDP11_CONSOLE_POWER_CONTROL_OFF;
        break;
    case PDP11_CONSOLE_POWER_CONTROL_LOCK:
        self->_power_control_switch = PDP11_CONSOLE_POWER_CONTROL_POWER;
        break;
    }
}

void pdp11_console_toggle_control_switch(
    Pdp11Console *const self,
    unsigned const i
) {
    if (self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_LOCK) return;
    self->_switch_register ^= 1 << i;
}

void pdp11_console_press_load_addr(Pdp11Console *const self) {
    if (self->_enable_switch ||
        self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_LOCK)
        return;

    self->_addr_register = self->_switch_register;

    self->_is_deposit_pressed_consecutively = false;
    self->_is_examine_pressed_consecutively = false;
}
void pdp11_console_press_deposit(Pdp11Console *const self) {
    if (self->_enable_switch ||
        self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_LOCK)
        return;

    if (self->_is_deposit_pressed_consecutively) self->_addr_register += 2;
    self->_is_deposit_pressed_consecutively = true;
    self->_is_examine_pressed_consecutively = false;

    self->_data_register = self->_switch_register;
    switch (self->_addr_register) {
    case PDP11_CONSOLE_CPU_REG_ADDRESS ... PDP11_CONSOLE_CPU_REG_ADDRESS +
        7 * 2 - 1: {
        unsigned const r_i = (self->_addr_register >> 1) & 07;
        pdp11_cpu_rx(&self->_pdp11->cpu, r_i) = self->_data_register;
    } break;
    default: {
        unibus_cpu_dato(
            &self->_pdp11->unibus,
            self->_addr_register,
            self->_data_register
        );
    } break;
    }
}
void pdp11_console_press_examine(Pdp11Console *const self) {
    if (self->_enable_switch ||
        self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_LOCK)
        return;

    if (self->_is_examine_pressed_consecutively) self->_addr_register += 2;
    self->_is_examine_pressed_consecutively = true;
    self->_is_deposit_pressed_consecutively = false;

    switch (self->_addr_register) {
    case PDP11_CONSOLE_CPU_REG_ADDRESS ... PDP11_CONSOLE_CPU_REG_ADDRESS +
        7 * 2 - 1: {
        unsigned const r_i = (self->_addr_register >> 1) & 07;
        self->_data_register = pdp11_cpu_rx(&self->_pdp11->cpu, r_i);
    } break;
    default: {
        unibus_cpu_dati(
            &self->_pdp11->unibus,
            self->_addr_register,
            &self->_data_register
        );
    } break;
    }
}

void pdp11_console_press_continue(Pdp11Console *const self) {
    if (self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_LOCK) return;

    if (self->_enable_switch) {
        pdp11_cpu_continue(&self->_pdp11->cpu);
    } else {
        pdp11_cpu_single_step(&self->_pdp11->cpu);
    }
}
void pdp11_console_toggle_enable(Pdp11Console *const self) {
    if (self->_enable_switch) {
        pdp11_cpu_halt(&self->_pdp11->cpu);
        self->_enable_switch = false;
    } else {
        self->_enable_switch = true;
    }
}
void pdp11_console_press_start(Pdp11Console *const self) {
    if (self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_LOCK) return;

    // NOTE after reset, `_addr_register` will become zero, so we cache it
    uint16_t const addr = self->_addr_register;

    unibus_reset(&self->_pdp11->unibus);
    pdp11_cpu_pc(&self->_pdp11->cpu) = addr;
    if (self->_enable_switch) pdp11_cpu_continue(&self->_pdp11->cpu);
}

bool pdp11_console_run_light(Pdp11Console const *const self) {
    return self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_OFF
             ? false
             : pdp11_cpu_state(&self->_pdp11->cpu) == PDP11_CPU_STATE_RUN &&
                   unibus_is_running(&self->_pdp11->unibus);
}
bool pdp11_console_bus_light(Pdp11Console const *const self) {
    return self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_OFF
             ? false
             : !self->_enable_switch &&
                   unibus_is_periph_master(&self->_pdp11->unibus);
}
bool pdp11_console_fetch_light(Pdp11Console const *const self) {
    return self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_OFF
             ? false
             : pdp11_console_simulated_light(self);
}
bool pdp11_console_exec_light(Pdp11Console const *const self) {
    return self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_OFF
             ? false
             : pdp11_console_simulated_light(self);
}
bool pdp11_console_source_light(Pdp11Console const *const self) {
    return self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_OFF
             ? false
             : pdp11_console_simulated_light(self);
}
bool pdp11_console_destination_light(Pdp11Console const *const self) {
    return self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_OFF
             ? false
             : pdp11_console_simulated_light(self);
}
unsigned pdp11_console_address_light(Pdp11Console const *const self) {
    return self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_OFF
             ? false
             : pdp11_console_simulated_light(self) << 1 |
                   pdp11_console_simulated_light(self);
}

void pdp11_console_insert_bootstrap(Pdp11Console *const self) {
    static uint16_t const bootstrap[] = {
        0016701,
        0000026,
        0012702,
        0000352,
        0005211,
        0105711,
        0100376,
        0116162,
        0000002,
        (PDP11_BOOTSTRAP_ADDR & ~07777) | 0007400,
        0005267,
        0177756,
        0000765,
        0177550,
    };

    if (self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_OFF ||
        self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_LOCK ||
        self->_enable_switch)
        return;

    pdp11_console_press_start(self);
    self->_switch_register = PDP11_BOOTSTRAP_ADDR;
    pdp11_console_press_load_addr(self);
    foreach (instr_ptr, bootstrap, bootstrap + lenof(bootstrap)) {
        self->_switch_register = *instr_ptr;
        pdp11_console_press_deposit(self);
    }

    self->_switch_register = PDP11_BOOTSTRAP_ADDR;
    pdp11_console_press_load_addr(self);
    pdp11_console_press_examine(self);
    pdp11_console_press_load_addr(self);
}

/***************
 ** interface **
 ***************/

static void pdp11_console_reset(Pdp11Console *const self) {
    self->_addr_register = self->_data_register = 0;

    self->_is_deposit_pressed_consecutively = false;
    self->_is_examine_pressed_consecutively = false;
}
static bool pdp11_console_try_read(
    Pdp11Console *const self,
    uint16_t const addr,
    uint16_t *const out
) {
    if (addr != PDP11_CONSOLE_SWITCH_REGISTER_ADDR) return false;

    *out = self->_switch_register;

    return true;
}
static bool pdp11_console_try_write_word(
    Pdp11Console *const,
    uint16_t const addr,
    uint16_t const
) {
    return addr == PDP11_CONSOLE_SWITCH_REGISTER_ADDR;
}
static bool pdp11_console_try_write_byte(
    Pdp11Console *const,
    uint16_t const addr,
    uint8_t const
) {
    return addr == PDP11_CONSOLE_SWITCH_REGISTER_ADDR;
}
UnibusDevice pdp11_console_ww_unibus_device(Pdp11Console *const self) {
    WRAP_BODY(
        UnibusDevice,
        UNIBUS_DEVICE_INTERFACE(Pdp11Console),
        {
            ._reset = pdp11_console_reset,
            ._try_read = pdp11_console_try_read,
            ._try_write_word = pdp11_console_try_write_word,
            ._try_write_byte = pdp11_console_try_write_byte,
        }
    );
}
