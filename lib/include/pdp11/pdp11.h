#ifndef PDP11_H
#define PDP11_H
#if (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
#  warning                                                                     \
      "This code was made to work on little-endian machines only, as PDP11 is little-endian itself."
#endif

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <assert.h>
#include <result.h>

#include "pdp11/cpu/pdp11_cpu.h"
#include "pdp11/pdp11_console.h"
#include "pdp11/pdp11_ram.h"

#define PDP11_RAM_SIZE (24 * 1024 * 2)

#define PDP11_PAPERTAPE_READER_ADDR          (0177550)
#define PDP11_PAPERTAPE_READER_INTR_VEC      (070)
#define PDP11_PAPERTAPE_READER_INTR_PRIORITY (04)

#define PDP11_TELETYPE_ADDR              (0177560)
#define PDP11_TELETYPE_KEYBOARD_INTR_VEC (060)
#define PDP11_TELETYPE_PRINTER_INTR_VEC  (064)
#define PDP11_TELETYPE_INTR_PRIORITY     (04)

typedef struct Pdp11 {
    Unibus unibus;
    Pdp11Cpu cpu;
    Pdp11Console console;

    Pdp11Ram ram;
    UnibusDevice *periphs;
} Pdp11;

Result pdp11_init(Pdp11 *const self);
void pdp11_uninit(Pdp11 *const self);

#endif
