#include "pdp11/pdp11_teletype.h"

#include <stdlib.h>

#include <unistd.h>

#include "bits.h"

/*************
 ** private **
 *************/

static uint16_t pdp11_teletype_keyboard_status_to_word(
    Pdp11TeletypeKeyboardStatus const self
) {
    return self.busy << 11 | self.done << 7 | self.intr_enable << 6;
}
static uint16_t pdp11_teletype_printer_status_to_word(
    Pdp11TeletypePrinterStatus const self
) {
    return self.ready << 7 | self.intr_enable << 6 | self.maintenance << 2;
}

static void pdp11_teletype_printer_thread_helper(Pdp11Teletype *const self) {
    FILE *const file = fopen("tty", "w");

    while (true) {
        while (self->_printer_status.ready) sleep(0);

        fputc(self->_printer_buffer, file), fflush(file);

        self->_printer_status.ready = true;

        // TODO bug is here
        if (0 && self->_keyboard_status.intr_enable)
            unibus_br_intr(
                self->_unibus,
                self->_intr_priority,
                self,
                self->_printer_intr_vec
            );

        usleep(1000000 / 10);
    }
}
static void *pdp11_teletype_printer_thread(void *const vself) {
    return pdp11_teletype_printer_thread_helper(vself), NULL;
};

/************
 ** public **
 ************/

Result pdp11_teletype_init(
    Pdp11Teletype *const self,
    Unibus *const unibus,
    uint16_t const starting_addr,
    uint8_t const keyboard_intr_vec,
    uint8_t const printer_intr_vec,
    unsigned const intr_priority,
    size_t const
) {
    // self->_buf = malloc(buf_len * elsizeof(self->_buf));
    // if (!self->_buf) return OutOfMemErr;

    self->_keyboard_status = (Pdp11TeletypeKeyboardStatus){0};
    self->_keyboar_buffer = 0;
    self->_printer_status = (Pdp11TeletypePrinterStatus){0};
    self->_printer_buffer = 0;

    self->_starting_addr = starting_addr;
    self->_keyboard_intr_vec = keyboard_intr_vec;
    self->_printer_intr_vec = printer_intr_vec;
    self->_intr_priority = intr_priority;

    self->_unibus = unibus;

    pthread_create(&self->_thread, NULL, pdp11_teletype_printer_thread, self);

    return Ok;
}
void pdp11_teletype_uninit(Pdp11Teletype *const self) {
    pthread_cancel(self->_thread);

    // free(self->_buf);
}

void pdp11_teletype_putc(Pdp11Teletype *const self, char const c) {
    self->_keyboard_status.busy = true;
    self->_keyboar_buffer = c;
    self->_keyboard_status.busy = false;

    self->_keyboard_status.done = true;

    if (self->_keyboard_status.intr_enable)
        unibus_br_intr(
            self->_unibus,
            self->_intr_priority,
            self,
            self->_keyboard_intr_vec
        );
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
    self->_printer_status = (Pdp11TeletypePrinterStatus){
        .ready = true,
        .intr_enable = false,
        .maintenance = false,
    };
}
static bool pdp11_teletype_try_read(
    Pdp11Teletype *const self,
    uint16_t addr,
    uint16_t *const out
) {
    addr -= self->_starting_addr;
    if (!(addr < 8)) return false;

    // NOTE odd addresses cannot pass here
    switch (addr) {
    case 0:
        *out = pdp11_teletype_keyboard_status_to_word(self->_keyboard_status);
        break;
    case 2: {
        self->_keyboard_status.done = false;
        *out = self->_keyboar_buffer;
    } break;
    case 4:
        *out = pdp11_teletype_printer_status_to_word(self->_printer_status);
        break;
    case 6: *out = 0; break;
    }
    return true;
}
static bool pdp11_teletype_try_write_word(
    Pdp11Teletype *const self,
    uint16_t addr,
    uint16_t const val
) {
    addr -= self->_starting_addr;
    if (!(addr < 8)) return false;

    // NOTE odd addresses cannot pass here
    switch (addr) {
    case 0: {
        self->_keyboard_status.intr_enable = BIT(val, 6);
        if (BIT(val, 0)) self->_keyboard_status.done = false;
    } break;
    case 2: self->_keyboard_status.done = false; break;
    case 4:
        self->_printer_status.intr_enable = BIT(val, 6);
        self->_printer_status.maintenance = BIT(val, 2);
        break;
    case 6:
        self->_printer_buffer = val;
        self->_printer_status.ready = false;
        break;
    }
    return true;
}
static bool pdp11_teletype_try_write_byte(
    Pdp11Teletype *const self,
    uint16_t addr,
    uint8_t const val
) {
    addr -= self->_starting_addr;
    if (!(addr < 8)) return false;

    switch (addr) {
    case 0: {
        self->_keyboard_status.intr_enable = BIT(val, 6);
        if (BIT(val, 0)) self->_keyboard_status.done = false;
    } break;
    case 1: break;
    case 2 ... 3: self->_keyboard_status.done = false; break;
    case 4:
        self->_printer_status.intr_enable = BIT(val, 6);
        self->_printer_status.maintenance = BIT(val, 2);
        break;
    case 5: break;
    case 6:
        self->_printer_buffer = val;
        self->_printer_status.ready = false;
        break;
    case 7: break;
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
