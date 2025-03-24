#ifndef PDP11_H
#define PDP11_H

#include <stdint.h>

#include "conviniences.h"

#if (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
#  warning                                                                     \
      "This code was made to work on little-endian machines only, as PDP11 is little-endian itself."
#endif

#include <result.h>

#define PDP11_REGISTER_COUNT (8)
// PDP11/20 has 32K words of RAM, but top 4096 are reserved
#define PDP11_RAM_WORD_COUNT (32 * 1024 - 4096)

#define PDP11_STARTUP_PC (0x0200)
#define PDP11_STARTUP_PS (0x00)

enum {
    PDP11_PS_C = 1 << 0,  // carry flag
    PDP11_PS_V = 1 << 1,  // overflow flag
    PDP11_PS_Z = 1 << 2,  // zero flag
    PDP11_PS_N = 1 << 3,  // negative flag
    // PDP11_PS_T = 1 << 4,  // trap (for debugging?)
};

typedef struct Pdp11 {
    struct {
        uint16_t r[PDP11_REGISTER_COUNT];
        uint8_t ps;
    } cpu;
    uint16_t *_ram;
} Pdp11;

Result pdp11_init(Pdp11 *const self);
void pdp11_uninit(Pdp11 *const self);

void pdp11_step(Pdp11 *const self);

#define pdp11_ram_word_at(SELF_, ADDR_)                                        \
    ((SELF_)->_ram[(ADDR_) / elsizeof((SELF_)->_ram)])

#define pdp11_ram_byte_at(SELF_, ADDR_) (((uint8_t *)(SELF_)->_ram)[ADDR_])

#endif
