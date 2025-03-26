#ifndef PDP11_H
#define PDP11_H

#include <stdbool.h>
#include <stdint.h>

#if (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
#  warning                                                                     \
      "This code was made to work on little-endian machines only, as PDP11 is little-endian itself."
#endif

#include <result.h>

#define PDP11_REGISTER_COUNT (8)
// PDP11/20 has 32K words of RAM, but top 4096 are reserved
#define PDP11_RAM_WORD_COUNT (32 * 1024 - 4096)

#define PDP11_STARTUP_PC (0x0200)
#define PDP11_STARTUP_PS                                                       \
  ((Pdp11Ps){.priority = 0, .tf = 0, .nf = 0, .zf = 0, .vf = 0, .cf = 0})

typedef struct Pdp11Ps {
    uint8_t priority : 3;
    bool tf : 1, nf : 1, zf : 1, vf : 1, cf : 1;
} Pdp11Ps;

typedef struct Pdp11 {
    struct {
        uint16_t r[PDP11_REGISTER_COUNT];
        Pdp11Ps ps;
    } _cpu;
    void *_ram;
} Pdp11;

Result pdp11_init(Pdp11 *const self);
void pdp11_uninit(Pdp11 *const self);

#define pdp11_rx(SELF_, I_) (*(uint16_t *)((SELF_)->_cpu.r + I_))
#define pdp11_rl(SELF_)     (*(uint8_t *)((SELF_)->_cpu.r + I_))
#define pdp11_pc(SELF_)     pdp11_rx(SELF_, 7)
// #define pdp11_sp(SELF_)     pdp11_rx(SELF_, 6)

#define pdp11_ps(SELF_) ((SELF_)->_cpu.ps)

#define pdp11_ram_word_at(SELF_, ADDR_) (*(uint16_t *)((SELF_)->_ram + (ADDR_)))
#define pdp11_ram_byte_at(SELF_, ADDR_) (*(uint8_t *)((SELF_)->_ram + (ADDR_)))

uint16_t pdp11_instr_next(Pdp11 *const self);

void pdp11_step(Pdp11 *const self);

#endif
