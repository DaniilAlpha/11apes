#include "pdp11/pdp11_papertape_reader.h"

#include <unistd.h>

#include "bits.h"

/*************
 ** private **
 *************/

static uint16_t pdp11_papertape_reader_status_to_word(
    Pdp11PapertapeReaderStatus const self
) {
    return self.error << 15 | self.busy << 11 | self.done << 7 |
           self.intr_enable << 6;
}

static void pdp11_papertape_reader_start_read_cycle(
    Pdp11PapertapeReader *const self
) {
    self->_status.busy = true;
    self->_status.done = false;
    self->_buffer = 0;
}

static void pdp11_papertape_reader_thread_helper(
    Pdp11PapertapeReader *const self
) {
    while (true) {
        while (!self->_status.busy) sleep(0);

        if (!self->_status.error) {
            if (!self->_tape || fread(&self->_buffer, 1, 1, self->_tape) != 1) {
                self->_status.error = true;
            } else {
                self->_status.done = true;
            }
            self->_status.busy = false;
        }

        if (self->_status.intr_enable)
            unibus_br_intr(
                self->_unibus,
                self->_intr_priority,
                self,
                self->_intr_vec
            );

        usleep(1000000 / 300);
    }
}
static void *pdp11_papertape_reader_thread(void *const vself) {
    return pdp11_papertape_reader_thread_helper(vself), NULL;
};

/************
 ** public **
 ************/

void pdp11_papertape_reader_init(
    Pdp11PapertapeReader *const self,
    Unibus *const unibus,
    uint16_t const starting_addr,
    uint8_t const intr_vec,
    unsigned const intr_priority

) {
    self->_tape = NULL;

    self->_status = (Pdp11PapertapeReaderStatus){0};
    self->_buffer = 0;

    self->_starting_addr = starting_addr;
    self->_intr_vec = intr_vec;
    self->_intr_priority = intr_priority;

    self->_unibus = unibus;

    pthread_create(&self->_thread, NULL, pdp11_papertape_reader_thread, self);
}
void pdp11_papertape_reader_uninit(Pdp11PapertapeReader *const self) {
    pthread_cancel(self->_thread);

    if (self->_tape) fclose(self->_tape), self->_tape = NULL;
}

void pdp11_papertape_reader_load(
    Pdp11PapertapeReader *const self,
    char const *const filepath
) {
    if (self->_tape) fclose(self->_tape), self->_tape = NULL;

    self->_tape = fopen(filepath, "r");
}

/***************
 ** interface **
 ***************/

static void pdp11_papertape_reader_reset(Pdp11PapertapeReader *const self) {
    self->_status = (Pdp11PapertapeReaderStatus){
        .error = false,
        .busy = false,
        .done = false,
        .intr_enable = false,
    };
}
static bool pdp11_papertape_reader_try_read(
    Pdp11PapertapeReader *const self,
    uint16_t addr,
    uint16_t *const out
) {
    addr -= self->_starting_addr;
    if (!(addr < 4)) return false;

    // NOTE odd addresses cannot pass here
    switch (addr) {
    case 0: *out = pdp11_papertape_reader_status_to_word(self->_status); break;
    case 2: {
        self->_status.done = false;
        *out = self->_buffer;
    } break;
    }
    return true;
}
static bool pdp11_papertape_reader_try_write_word(
    Pdp11PapertapeReader *const self,
    uint16_t addr,
    uint16_t const val
) {
    addr -= self->_starting_addr;
    if (!(addr < 4)) return false;

    // NOTE odd addresses cannot pass here
    switch (addr) {
    case 0: {
        self->_status.intr_enable = BIT(val, 6);
        if (BIT(val, 0)) pdp11_papertape_reader_start_read_cycle(self);
    } break;
    case 2: break;
    }
    return true;
}
static bool pdp11_papertape_reader_try_write_byte(
    Pdp11PapertapeReader *const self,
    uint16_t addr,
    uint8_t const val
) {
    addr -= self->_starting_addr;
    if (!(addr < 4)) return false;

    switch (addr) {
    case 0: {
        self->_status.intr_enable = BIT(val, 6);
        if (BIT(val, 0)) pdp11_papertape_reader_start_read_cycle(self);
    } break;
    case 1 ... 3: break;
    }
    return true;
}
UnibusDevice pdp11_papertape_reader_ww_unibus_device(
    Pdp11PapertapeReader *const self
) {
    WRAP_BODY(
        UnibusDevice,
        UNIBUS_DEVICE_INTERFACE(Pdp11PapertapeReader),
        {
            ._reset = pdp11_papertape_reader_reset,
            ._try_read = pdp11_papertape_reader_try_read,
            ._try_write_word = pdp11_papertape_reader_try_write_word,
            ._try_write_byte = pdp11_papertape_reader_try_write_byte,
        }
    );
}
