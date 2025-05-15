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

        bool is_error = self->_status.error;
        uint8_t buffer = 0;
        if (!is_error) {
            if (!self->_tape || fread(&buffer, 1, 1, self->_tape) != 1)
                is_error = true;
        }

        pthread_mutex_lock(&self->_lock);
        {
            self->_status.busy = false;
            self->_status.error = is_error;
            self->_status.done = !is_error;
            if (!is_error) self->_buffer = buffer;

            if (self->_status.intr_enable)
                unibus_br_intr(
                    self->_unibus,
                    self->_intr_priority,
                    self,
                    self->_intr_vec
                );
        }
        pthread_mutex_unlock(&self->_lock);

        usleep(1000000 / 300 / 10);
    }
}
static void *pdp11_papertape_reader_thread(void *const vself) {
    return pdp11_papertape_reader_thread_helper(vself), NULL;
};

/************
 ** public **
 ************/

Result pdp11_papertape_reader_init(
    Pdp11PapertapeReader *const self,
    Unibus *const unibus,
    uint16_t const starting_addr,
    uint8_t const intr_vec,
    unsigned const intr_priority

) {
    pthread_mutex_init(&self->_lock, NULL);
    if (pthread_create(
            &self->_thread,
            NULL,
            pdp11_papertape_reader_thread,
            self
        ) != 0)
        return UnknownErr;

    self->_tape = NULL;

    self->_status = (Pdp11PapertapeReaderStatus){0};
    self->_buffer = 0;

    self->_starting_addr = starting_addr;
    self->_intr_vec = intr_vec;
    self->_intr_priority = intr_priority;

    self->_unibus = unibus;

    return Ok;
}
void pdp11_papertape_reader_uninit(Pdp11PapertapeReader *const self) {
    pthread_cancel(self->_thread);
    pthread_mutex_destroy(&self->_lock);

    if (self->_tape) fclose(self->_tape), self->_tape = NULL;
}

Result pdp11_papertape_reader_load(
    Pdp11PapertapeReader *const self,
    char const *const filepath
) {
    FILE *const old_file = self->_tape;
    self->_tape = fopen(filepath, "r");
    if (!self->_tape) return self->_tape = old_file, FileUnavailableErr;

    if (old_file) fclose(old_file);
    return Ok;
}

/***************
 ** interface **
 ***************/

static void pdp11_papertape_reader_reset(Pdp11PapertapeReader *const self) {
    pthread_mutex_lock(&self->_lock);
    {
        self->_status = (Pdp11PapertapeReaderStatus){
            .error = false,
            .busy = false,
            .done = false,
            .intr_enable = false,
        };
        self->_buffer = 0;
    }
    pthread_mutex_unlock(&self->_lock);
}
static bool pdp11_papertape_reader_try_read(
    Pdp11PapertapeReader *const self,
    uint16_t addr,
    uint16_t *const out
) {
    addr -= self->_starting_addr;
    if (!(addr < 4)) return false;

    pthread_mutex_lock(&self->_lock);
    {
        // NOTE odd addresses cannot pass here
        switch (addr) {
        case 0:
            *out = pdp11_papertape_reader_status_to_word(self->_status);
            break;
        case 2: {
            self->_status.done = false;
            *out = self->_buffer;
        } break;
        }
    }
    pthread_mutex_unlock(&self->_lock);
    return true;
}
static bool pdp11_papertape_reader_try_write_word(
    Pdp11PapertapeReader *const self,
    uint16_t addr,
    uint16_t const val
) {
    addr -= self->_starting_addr;
    if (!(addr < 4)) return false;

    pthread_mutex_lock(&self->_lock);
    {
        // NOTE odd addresses cannot pass here
        switch (addr) {
        case 0: {
            self->_status.intr_enable = BIT(val, 6);
            if (BIT(val, 0)) pdp11_papertape_reader_start_read_cycle(self);
        } break;
        case 2: self->_status.done = false; break;
        }
    }
    pthread_mutex_unlock(&self->_lock);
    return true;
}
static bool pdp11_papertape_reader_try_write_byte(
    Pdp11PapertapeReader *const self,
    uint16_t addr,
    uint8_t const val
) {
    addr -= self->_starting_addr;
    if (!(addr < 4)) return false;

    pthread_mutex_lock(&self->_lock);
    {
        switch (addr) {
        case 0: {
            self->_status.intr_enable = BIT(val, 6);
            if (BIT(val, 0)) pdp11_papertape_reader_start_read_cycle(self);
        } break;
        case 1: break;
        case 2 ... 3: self->_status.done = false; break;
        }
    }
    pthread_mutex_unlock(&self->_lock);
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
