#include "pdp11/pdp11_console.h"

#include <stdlib.h>

void pdp11_console_init(Pdp11Console *const self, Pdp11 *const pdp11) {
    self->_pdp11 = pdp11;

    self->_switch_register = 0;
    self->_addr_register = self->_data_register = 0;

    self->_enable_switch = true;
    self->_power_control_switch = PDP11_CONSOLE_POWER_CONTROL_OFF;

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

void pdp11_console_next_power_control(Pdp11Console *const self) {
    switch (self->_power_control_switch) {
    case PDP11_CONSOLE_POWER_CONTROL_OFF: {
        pdp11_power_up(self->_pdp11);
        self->_power_control_switch = PDP11_CONSOLE_POWER_CONTROL_POWER;
    } break;

    case PDP11_CONSOLE_POWER_CONTROL_POWER: {
        self->_power_control_switch = PDP11_CONSOLE_POWER_CONTROL_LOCK;
    } break;

    case PDP11_CONSOLE_POWER_CONTROL_LOCK: break;
    }
}
void pdp11_console_prev_power_control(Pdp11Console *const self) {
    switch (self->_power_control_switch) {
    case PDP11_CONSOLE_POWER_CONTROL_OFF: break;

    case PDP11_CONSOLE_POWER_CONTROL_POWER: {
        pdp11_power_down(self->_pdp11);
        self->_power_control_switch = PDP11_CONSOLE_POWER_CONTROL_OFF;
    } break;

    case PDP11_CONSOLE_POWER_CONTROL_LOCK: {
        self->_power_control_switch = PDP11_CONSOLE_POWER_CONTROL_POWER;
    } break;
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
    unibus_dato(
        &self->_pdp11->unibus,
        UNIBUS_DEVICE_CPU,
        self->_addr_register,
        self->_data_register
    );
}
void pdp11_console_press_examine(Pdp11Console *const self) {
    if (self->_enable_switch ||
        self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_LOCK)
        return;

    if (self->_is_examine_pressed_consecutively) self->_addr_register += 2;
    self->_is_examine_pressed_consecutively = true;
    self->_is_deposit_pressed_consecutively = false;

    self->_data_register = unibus_dati(
        &self->_pdp11->unibus,
        UNIBUS_DEVICE_CPU,
        self->_addr_register
    );
}

void pdp11_console_press_continue(Pdp11Console *const self) {
    if (self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_LOCK) return;

    // TODO system clear
    if (self->_enable_switch) {
        // TODO restart programm at the last loaded address
    }
}
void pdp11_console_toggle_enable(Pdp11Console *const self) {
    if (self->_enable_switch) {
        // TODO! halt
        self->_enable_switch = false;
    } else {
        // TODO? what?
        self->_enable_switch = true;
    }
}
void pdp11_console_press_start(Pdp11Console *const self) {
    if (self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_LOCK) return;

    if (self->_enable_switch) {
        // TODO unhalt and continue
    } else {
        // TODO single step programm
    }
}

bool pdp11_console_run_light(Pdp11Console const *const self) {
    if (self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_OFF)
        return false;
    return self->_enable_switch || unibus_is_running(&self->_pdp11->unibus);
}
bool pdp11_console_bus_light(Pdp11Console const *const self) {
    if (self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_OFF)
        return false;
    return !self->_enable_switch &&
           unibus_is_periph_master(&self->_pdp11->unibus);
}
bool pdp11_console_fetch_light(Pdp11Console const *const self) {
    if (self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_OFF)
        return false;
    return self->_enable_switch && (rand() & 1);
}
bool pdp11_console_exec_light(Pdp11Console const *const self) {
    if (self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_OFF)
        return false;
    return self->_enable_switch && (rand() & 1);
}
bool pdp11_console_source_light(Pdp11Console const *const self) {
    if (self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_OFF)
        return false;
    return self->_enable_switch && (rand() & 1);
}
bool pdp11_console_destination_light(Pdp11Console const *const self) {
    if (self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_OFF)
        return false;
    return self->_enable_switch && (rand() & 1);
}
unsigned pdp11_console_address_light(Pdp11Console const *const self) {
    if (self->_power_control_switch == PDP11_CONSOLE_POWER_CONTROL_OFF)
        return false;
    return self->_enable_switch ? (rand() & 1) : 0;
}

UnibusDevice pdp11_console_ww_unibus_device(Pdp11Console *const self) {
    WRAP_BODY(
        UnibusDevice,
        UNIBUS_DEVICE_INTERFACE(Pdp11Console),
        {
            ._try_read = pdp11_console_try_read,
            ._try_write_word = pdp11_console_try_write_word,
            ._try_write_byte = pdp11_console_try_write_byte,
        }
    );
}
