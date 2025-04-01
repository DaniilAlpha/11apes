#ifndef PDP11_CPU_INSTR_SET_H
#define PDP11_CPU_INSTR_SET_H

#include <stdalign.h>

#include "conviniences.h"
#include "pdp11/cpu/pdp11_cpu.h"
#include "pdp11/pdp11_ram.h"

// SINGLE-OP

// general

static forceinline void pdp11_op_clr(Pdp11Cpu *const self, uint16_t *const dst);
static forceinline void pdp11_op_clrb(Pdp11Cpu *const self, uint8_t *const dst);
static forceinline void pdp11_op_inc(Pdp11Cpu *const self, uint16_t *const dst);
static forceinline void pdp11_op_incb(Pdp11Cpu *const self, uint8_t *const dst);
static forceinline void pdp11_op_dec(Pdp11Cpu *const self, uint16_t *const dst);
static forceinline void pdp11_op_decb(Pdp11Cpu *const self, uint8_t *const dst);

static forceinline void pdp11_op_neg(Pdp11Cpu *const self, uint16_t *const dst);
static forceinline void pdp11_op_negb(Pdp11Cpu *const self, uint8_t *const dst);

static forceinline void
pdp11_op_tst(Pdp11Cpu *const self, uint16_t const *const src);
static forceinline void
pdp11_op_tstb(Pdp11Cpu *const self, uint8_t const *const src);

// NOTE this is a `COMplement` instruction, like `not` in intel
static forceinline void pdp11_op_com(Pdp11Cpu *const self, uint16_t *const dst);
static forceinline void pdp11_op_comb(Pdp11Cpu *const self, uint8_t *const dst);

// shifts

static forceinline void pdp11_op_asr(Pdp11Cpu *const self, uint16_t *const dst);
static forceinline void pdp11_op_asrb(Pdp11Cpu *const self, uint8_t *const dst);
static forceinline void pdp11_op_asl(Pdp11Cpu *const self, uint16_t *const dst);
static forceinline void pdp11_op_aslb(Pdp11Cpu *const self, uint8_t *const dst);

static forceinline void pdp11_op_ash(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
);
static forceinline void pdp11_op_ashc(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
);

// multiple-percision

static forceinline void pdp11_op_adc(Pdp11Cpu *const self, uint16_t *const dst);
static forceinline void pdp11_op_adcb(Pdp11Cpu *const self, uint8_t *const dst);
static forceinline void pdp11_op_sbc(Pdp11Cpu *const self, uint16_t *const dst);
static forceinline void pdp11_op_sbcb(Pdp11Cpu *const self, uint8_t *const dst);

static forceinline void pdp11_op_sxt(Pdp11Cpu *const self, uint16_t *const dst);

// rotates

static forceinline void pdp11_op_ror(Pdp11Cpu *const self, uint16_t *const dst);
static forceinline void pdp11_op_rorb(Pdp11Cpu *const self, uint8_t *const dst);
static forceinline void pdp11_op_rol(Pdp11Cpu *const self, uint16_t *const dst);
static forceinline void pdp11_op_rolb(Pdp11Cpu *const self, uint8_t *const dst);

static forceinline void
pdp11_op_swab(Pdp11Cpu *const self, uint16_t *const dst);

// DUAL-OP

// arithmetic

static forceinline void pdp11_op_mov(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
);
static forceinline void pdp11_op_movb(
    Pdp11Cpu *const self,
    uint8_t const *const src,
    uint8_t *const dst
);
static forceinline void pdp11_op_add(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
);
static forceinline void pdp11_op_sub(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
);
static forceinline void pdp11_op_cmp(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t const *const dst
);
static forceinline void pdp11_op_cmpb(
    Pdp11Cpu *const self,
    uint8_t const *const src,
    uint8_t const *const dst
);

// register destination

static forceinline void pdp11_op_mul(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
);
static forceinline void pdp11_op_div(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
);

static forceinline void
pdp11_op_xor(Pdp11Cpu *const self, unsigned const r_i, uint16_t *const dst);

// logical

static forceinline void pdp11_op_bit(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t const *const dst
);
static forceinline void pdp11_op_bitb(
    Pdp11Cpu *const self,
    uint8_t const *const src,
    uint8_t const *const dst
);
static forceinline void pdp11_op_bis(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
);
static forceinline void pdp11_op_bisb(
    Pdp11Cpu *const self,
    uint8_t const *const src,
    uint8_t *const dst
);
static forceinline void pdp11_op_bic(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
);
static forceinline void pdp11_op_bicb(
    Pdp11Cpu *const self,
    uint8_t const *const src,
    uint8_t *const dst
);

// PROGRAM CONTROL

// branches

static forceinline void pdp11_op_br(Pdp11Cpu *const self, uint8_t const off);

static forceinline void
pdp11_op_bne_be(Pdp11Cpu *const self, bool const cond, uint8_t const off);
static forceinline void
pdp11_op_bpl_bmi(Pdp11Cpu *const self, bool const cond, uint8_t const off);
static forceinline void
pdp11_op_bcc_bcs(Pdp11Cpu *const self, bool const cond, uint8_t const off);
static forceinline void
pdp11_op_bvc_bvs(Pdp11Cpu *const self, bool const cond, uint8_t const off);

static forceinline void
pdp11_op_bge_bl(Pdp11Cpu *const self, bool const cond, uint8_t const off);
static forceinline void
pdp11_op_bg_ble(Pdp11Cpu *const self, bool const cond, uint8_t const off);

static forceinline void
pdp11_op_bhi_blos(Pdp11Cpu *const self, bool const cond, uint8_t const off);
// NOTE `bhis`/`blo` is already implemented with `bcc`/`bcs`

// subroutine

static forceinline void pdp11_op_jsr(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
);
static forceinline void
pdp11_op_mark(Pdp11Cpu *const self, unsigned const param_count);
static forceinline void pdp11_op_rts(Pdp11Cpu *const self, unsigned const r_i);

// program control

static forceinline void
pdp11_op_spl(Pdp11Cpu *const self, unsigned const value);

static forceinline void
pdp11_op_jmp(Pdp11Cpu *const self, uint16_t const *const src);
static forceinline void
pdp11_op_sob(Pdp11Cpu *const self, unsigned const r_i, uint8_t const off);

// traps

static forceinline void pdp11_op_emt(Pdp11Cpu *const self);
static forceinline void pdp11_op_trap(Pdp11Cpu *const self);
static forceinline void pdp11_op_bpt(Pdp11Cpu *const self);
static forceinline void pdp11_op_iot(Pdp11Cpu *const self);
static forceinline void pdp11_op_rti(Pdp11Cpu *const self);
static forceinline void pdp11_op_rtt(Pdp11Cpu *const self);

// MISC.

static forceinline void pdp11_op_halt(Pdp11Cpu *const self);
static forceinline void pdp11_op_wait(Pdp11Cpu *const self);
static forceinline void pdp11_op_reset(Pdp11Cpu *const self);
// NOTE `nop` is already implemented with `clnzvc`/`senzvc`

static forceinline void
pdp11_op_mtpd(Pdp11Cpu *const self, uint16_t *const dst);
static forceinline void
pdp11_op_mtpi(Pdp11Cpu *const self, uint16_t *const dst);
static forceinline void
pdp11_op_mfpd(Pdp11Cpu *const self, uint16_t const *const src);
static forceinline void
pdp11_op_mfpi(Pdp11Cpu *const self, uint16_t const *const src);

// CONDITION CODES

static forceinline void pdp11_op_clnzvc_senzvc(
    Pdp11Cpu *const self,
    bool const value,
    bool const do_affect_nf,
    bool const do_affect_zf,
    bool const do_affect_vf,
    bool const do_affect_cf
);

#endif
