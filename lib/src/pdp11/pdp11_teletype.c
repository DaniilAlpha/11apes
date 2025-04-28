#include "pdp11/pdp11_teletype.h"

#include <stdlib.h>

#include <unistd.h>

#include "bits.h"
#include "conviniences.h"

/*************
 ** private **
 *************/

static uint16_t pdp11_teletype_keyboard_status_to_word(
    Pdp11TeletypeKeyboardStatus const self
) {
    return self.busy << 11 | self.done << 7 | self.intr_enable << 6;
}
static uint16_t pdp11_teletype_punsh_status_to_word(
    Pdp11TeletypePunchStatus const self
) {
    return self.ready << 7 | self.intr_enable << 6 | self.maintenance << 2;
}

static void pdp11_teletype_thread_helper(Pdp11Teletype *const self) {
    while (true) {
        while (!self->_keyboard_status.busy) sleep(0);

        if (!self->_keyboard_status.error) {
            if (!self->_tape ||
                fread(&self->_keyboard_buffer, 1, 1, self->_tape) != 1) {
                self->_keyboard_status.error = true;
            } else {
                self->_keyboard_status.done = true;
            }
            self->_keyboard_status.busy = false;
        }

        if (self->_keyboard_status.intr_enable)
            unibus_br_intr(
                self->_unibus,
                self->_intr_priority,
                self,
                self->_intr_vec
            );

        usleep(1000000 / 300);
    }
}
static void *pdp11_teletype_thread(void *const vself) {
    return pdp11_teletype_thread_helper(vself), NULL;
};

/************
 ** public **
 ************/

Result pdp11_teletype_init(
    Pdp11Teletype *const self,
    Unibus *const unibus,
    uint16_t const starting_addr,
    uint8_t const intr_vec,
    unsigned const intr_priority,
    size_t const buf_len
) {
    self->_buf = malloc(buf_len * elsizeof(self->_buf));
    if (!self->_buf) return OutOfMemErr;

    self->_keyboard_status = (Pdp11TeletypeKeyboardStatus){0};
    self->_keyboar_buffer = 0;
    self->_punch_status = (Pdp11TeletypePunchStatus){0};
    self->_punch_buffer = 0;

    self->_starting_addr = starting_addr;
    self->_intr_vec = intr_vec;
    self->_intr_priority = intr_priority;

    self->_unibus = unibus;

    pthread_create(&self->_thread, NULL, pdp11_teletype_thread, self);
}
void pdp11_teletype_uninit(Pdp11Teletype *const self) {
    pthread_cancel(self->_thread);

    free(self->_buf);
}

void pdp11_teletype_putc(Pdp11Teletype *const self, char const c) {
    // TODO! implement
}

/***************
 ** interface **
 ***************/

static void pdp11_teletype_reset(Pdp11Teletype *const self) {
    self->_keyboard_status = (Pdp11TeletypeKeyboardStatus){
        .busy = false,
        .done = false,
        .intr_enable = false,
    };
    self->_keyboar_buffer = 0;

    self->_punch_status = (Pdp11TeletypePunchStatus){
        .ready = true,
        .intr_enable = false,
        .maintenance = false,
    };
    self->_punch_buffer = 0;
}
static bool pdp11_teletype_try_read(
    Pdp11Teletype *const self,
    uint16_t addr,
    uint16_t *const out
) {
    addr -= self->_starting_addr;
    if (!(addr < 4)) return false;

    // NOTE odd addresses cannot pass here
    switch (addr) {
    case 0:
        *out = pdp11_teletype_keyboard_status_to_word(self->_keyboard_status);
        break;
    case 2: {
        *out = self->_keyboar_buffer;
    } break;
    case 4:
        *out = pdp11_teletype_punsh_status_to_word(self->_punch_status);
        break;
    case 6: {
        *out = self->_punch_buffer;
    } break;
    }
    return true;
}
static bool pdp11_teletype_try_write_word(
    Pdp11Teletype *const self,
    uint16_t addr,
    uint16_t const val
) {
    addr -= self->_starting_addr;
    if (!(addr < 4)) return false;

    // NOTE odd addresses cannot pass here
    switch (addr) {
    case 0: {
        self->_status.intr_enable = BIT(val, 6);
        if (BIT(val, 0)) pdp11_teletype_start_read_cycle(self);
    } break;
    case 2: break;
    }
    return true;
}
static bool pdp11_teletype_try_write_byte(
    Pdp11Teletype *const self,
    uint16_t addr,
    uint8_t const val
) {
    addr -= self->_starting_addr;
    if (!(addr < 4)) return false;

    switch (addr) {
    case 0: {
        self->_status.intr_enable = BIT(val, 6);
        if (BIT(val, 0)) pdp11_teletype_start_read_cycle(self);
    } break;
    case 1 ... 3: break;
    }
    return true;
}
UnibusDevice pdp11_teletype_ww_unibus_device(Pdp11Teletype *const self) {
    WRAP_BODY(
        UnibusDevice,
        UNIBUS_DEVICE_INTERFACE(Pdp11Teletype),
        {
            ._reset = pdp11_teletype_reset,
            ._try_read = pdp11_teletype_try_read,
            ._try_write_word = pdp11_teletype_try_write_word,
            ._try_write_byte = pdp11_teletype_try_write_byte,
        }
    );
}
