#ifndef PDP11_PAPERTAPE_READER_H
#define PDP11_PAPERTAPE_READER_H

#include <pthread.h>
#include <stdio.h>

#include "pdp11/unibus/unibus.h"
#include "pdp11/unibus/unibus_device.h"

typedef struct Pdp11PapertapeReaderStatus {
    bool error : 1;
    uint16_t : 3;
    bool busy : 1;
    uint16_t : 3;
    bool done : 1;
    bool intr_enable : 1;
    uint16_t : 5;
    bool : 1;
} Pdp11PapertapeReaderStatus;

typedef struct Pdp11PapertapeReader {
    Pdp11PapertapeReaderStatus _status;
    uint8_t _buffer;

    FILE *_tape;

    uint16_t _starting_addr;
    uint8_t _intr_vec;
    unsigned _intr_priority;

    Unibus *_unibus;

    pthread_t _thread;
} Pdp11PapertapeReader;

void pdp11_papertape_reader_init(
    Pdp11PapertapeReader *const self,
    Unibus *const unibus,
    uint16_t const starting_addr,
    uint8_t const intr_vec,
    unsigned const intr_priority
);
void pdp11_papertape_reader_uninit(Pdp11PapertapeReader *const self);

void pdp11_papertape_reader_load(
    Pdp11PapertapeReader *const self,
    char const *const filepath
);

UnibusDevice pdp11_papertape_reader_ww_unibus_device(
    Pdp11PapertapeReader *const self
);

#endif
