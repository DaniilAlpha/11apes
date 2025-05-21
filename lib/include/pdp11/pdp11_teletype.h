#ifndef PDP11_TELETYPE_H
#define PDP11_TELETYPE_H

#include <pthread.h>
#include <stdio.h>

#include "pdp11/unibus/unibus.h"
#include "pdp11/unibus/unibus_device.h"

typedef struct Pdp11TeletypeKeyboardStatus {
    uint16_t : 4;
    bool : 1;
    uint16_t : 3;
    bool done : 1;
    bool intr_enable : 1;
    uint16_t : 5;
    bool : 1;
} Pdp11TeletypeKeyboardStatus;

typedef struct Pdp11TeletypePunchStatus {
    uint16_t : 8;
    bool ready : 1;
    bool intr_enable : 1;
    uint16_t : 3;
    bool maintenance : 1;
    uint16_t : 2;
} Pdp11TeletypePrinterStatus;

typedef struct Pdp11Teletype {
    Pdp11TeletypeKeyboardStatus _keyboard_status;
    uint8_t _keyboar_buffer;
    Pdp11TeletypePrinterStatus _printer_status;
    uint8_t _printer_buffer;

    char *_buf;

    uint16_t _starting_addr;
    uint8_t _keyboard_intr_vec, _printer_intr_vec;
    unsigned _intr_priority;

    Unibus *_unibus;

    pthread_t _thread;
    pthread_mutex_t _keyboard_lock, _printer_lock;
} Pdp11Teletype;

Result pdp11_teletype_init(
    Pdp11Teletype *const self,
    Unibus *const unibus,
    uint16_t const starting_addr,
    uint8_t const keyboard_intr_vec,
    uint8_t const printer_intr_vec,
    unsigned const intr_priority
);
void pdp11_teletype_uninit(Pdp11Teletype *const self);

void pdp11_teletype_putc(Pdp11Teletype *const self, char const c);

UnibusDevice pdp11_teletype_ww_unibus_device(Pdp11Teletype *const self);

#endif
