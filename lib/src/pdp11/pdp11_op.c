#include "pdp11/pdp11_op.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "bits.h"
#include "conviniences.h"

// extends a word with an additional sign bit for overflow detection. never try
// to negate it.
static inline uint32_t xword(uint16_t const word) {
    return (word & 0x8000 ? 0x10000 : 0x00000) | word;
}
// extends a byte with an additional sign bit for overflow detection. never try
// to negate it.
static inline uint16_t xbyte(uint8_t const byte) {
    return (byte & 0x80 ? 0x100 : 0x000) | byte;
}

/****************
 ** instr decl **
 ****************/

// SINGLE-OP

// general

static forceinline void pdp11_op_clr(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_clrb(Pdp11 *const self, uint8_t *const dst);
static forceinline void pdp11_op_inc(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_incb(Pdp11 *const self, uint8_t *const dst);
static forceinline void pdp11_op_dec(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_decb(Pdp11 *const self, uint8_t *const dst);

static forceinline void pdp11_op_neg(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_negb(Pdp11 *const self, uint8_t *const dst);

static forceinline void
pdp11_op_tst(Pdp11 *const self, uint16_t const *const src);
static forceinline void
pdp11_op_tstb(Pdp11 *const self, uint8_t const *const src);

// NOTE this is a `COMplement` instruction, like `not` in intel
static forceinline void pdp11_op_com(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_comb(Pdp11 *const self, uint8_t *const dst);

// shifts

static forceinline void pdp11_op_asr(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_asrb(Pdp11 *const self, uint8_t *const dst);
static forceinline void pdp11_op_asl(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_aslb(Pdp11 *const self, uint8_t *const dst);

static forceinline void
pdp11_op_ash(Pdp11 *const self, unsigned const r_i, uint16_t *const dst);
static forceinline void
pdp11_op_ashc(Pdp11 *const self, unsigned const r_i, uint16_t *const dst);

// multiple-percision

static forceinline void pdp11_op_adc(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_adcb(Pdp11 *const self, uint8_t *const dst);
static forceinline void pdp11_op_sbc(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_sbcb(Pdp11 *const self, uint8_t *const dst);

static forceinline void pdp11_op_sxt(Pdp11 *const self, uint16_t *const dst);

// rotates

static forceinline void pdp11_op_ror(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_rorb(Pdp11 *const self, uint8_t *const dst);
static forceinline void pdp11_op_rol(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_rolb(Pdp11 *const self, uint8_t *const dst);

static forceinline void pdp11_op_swab(Pdp11 *const self, uint16_t *const dst);

// DUAL-OP

// arithmetic

static forceinline void
pdp11_op_mov(Pdp11 *const self, uint16_t const *const src, uint16_t *const dst);
static forceinline void
pdp11_op_movb(Pdp11 *const self, uint8_t const *const src, uint8_t *const dst);
static forceinline void
pdp11_op_add(Pdp11 *const self, uint16_t const *const src, uint16_t *const dst);
static forceinline void
pdp11_op_sub(Pdp11 *const self, uint16_t const *const src, uint16_t *const dst);
static forceinline void pdp11_op_cmp(
    Pdp11 *const self,
    uint16_t const *const src,
    uint16_t const *const dst
);
static forceinline void pdp11_op_cmpb(
    Pdp11 *const self,
    uint8_t const *const src,
    uint8_t const *const dst
);

// register destination

static forceinline void
pdp11_op_mul(Pdp11 *const self, unsigned const r_i, uint16_t const *const src);
static forceinline void
pdp11_op_div(Pdp11 *const self, unsigned const r_i, uint16_t const *const src);

static forceinline void
pdp11_op_xor(Pdp11 *const self, unsigned const r_i, uint16_t *const dst);

// logical

static forceinline void pdp11_op_bit(
    Pdp11 *const self,
    uint16_t const *const src,
    uint16_t const *const dst
);
static forceinline void pdp11_op_bitb(
    Pdp11 *const self,
    uint8_t const *const src,
    uint8_t const *const dst
);
static forceinline void
pdp11_op_bis(Pdp11 *const self, uint16_t const *const src, uint16_t *const dst);
static forceinline void
pdp11_op_bisb(Pdp11 *const self, uint8_t const *const src, uint8_t *const dst);
static forceinline void
pdp11_op_bic(Pdp11 *const self, uint16_t const *const src, uint16_t *const dst);
static forceinline void
pdp11_op_bicb(Pdp11 *const self, uint8_t const *const src, uint8_t *const dst);

// PROGRAM CONTROL

// branches

static forceinline void pdp11_op_br(Pdp11 *const self, uint8_t const off);

static forceinline void
pdp11_op_bne_be(Pdp11 *const self, bool const cond, uint8_t const off);
static forceinline void
pdp11_op_bpl_bmi(Pdp11 *const self, bool const cond, uint8_t const off);
static forceinline void
pdp11_op_bcc_bcs(Pdp11 *const self, bool const cond, uint8_t const off);
static forceinline void
pdp11_op_bvc_bvs(Pdp11 *const self, bool const cond, uint8_t const off);

static forceinline void
pdp11_op_bge_bl(Pdp11 *const self, bool const cond, uint8_t const off);
static forceinline void
pdp11_op_bg_ble(Pdp11 *const self, bool const cond, uint8_t const off);

static forceinline void
pdp11_op_bhi_blos(Pdp11 *const self, bool const cond, uint8_t const off);
// NOTE `bhis`/`blo` is already implemented with `bcc`/`bcs`

// subroutine

static forceinline void
pdp11_op_jsr(Pdp11 *const self, unsigned const r_i, uint16_t const *const src);
static forceinline void
pdp11_op_mark(Pdp11 *const self, unsigned const param_count);
static forceinline void pdp11_op_rts(Pdp11 *const self, unsigned const r_i);

// program control

static forceinline void pdp11_op_spl(Pdp11 *const self, unsigned const value);

static forceinline void
pdp11_op_jmp(Pdp11 *const self, uint16_t const *const src);
static forceinline void
pdp11_op_sob(Pdp11 *const self, unsigned const r_i, uint8_t const off);

// traps

static forceinline void
pdp11_op_emt_trap(Pdp11 *const self, bool const is_trap, uint8_t const code);
static forceinline void pdp11_op_bpt(Pdp11 *const self);
static forceinline void pdp11_op_iot(Pdp11 *const self);
static forceinline void pdp11_op_rti(Pdp11 *const self);
static forceinline void pdp11_op_rtt(Pdp11 *const self);

// MISC.

static forceinline void pdp11_op_halt(Pdp11 *const self);
static forceinline void pdp11_op_wait(Pdp11 *const self);
static forceinline void pdp11_op_reset(Pdp11 *const self);
// NOTE `nop` is already implemented with `clnzvc`/`senzvc`

static forceinline void pdp11_op_mtpd(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_mtpi(Pdp11 *const self, uint16_t *const dst);
static forceinline void
pdp11_op_mfpd(Pdp11 *const self, uint16_t const *const src);
static forceinline void
pdp11_op_mfpi(Pdp11 *const self, uint16_t const *const src);

// CONDITION CODES

static forceinline void pdp11_op_clnzvc_senzvc(
    Pdp11 *const self,
    bool const value,
    bool const do_affect_nf,
    bool const do_affect_zf,
    bool const do_affect_vf,
    bool const do_affect_cf
);

/*************
 ** private **
 *************/

static inline void
pdp11_ps_set_flags_from_word(Pdp11Ps *const self, uint16_t const value) {
    *self = (Pdp11Ps){
        .priority = self->priority,
        .tf = self->tf,
        .nf = BIT(value, 15),
        .zf = value == 0,
        .vf = 0,
        .cf = self->cf,
    };
}
static inline void
pdp11_ps_set_flags_from_byte(Pdp11Ps *const self, uint8_t const value) {
    *self = (Pdp11Ps){
        .priority = self->priority,
        .tf = self->tf,
        .nf = BIT(value, 7),
        .zf = value == 0,
        .vf = 0,
        .cf = self->cf,
    };
}

static inline void pdp11_ps_set_flags_from_xword(
    Pdp11Ps *const self,
    uint32_t const value,
    bool const do_invert_cf
) {
    *self = (Pdp11Ps){
        .priority = self->priority,
        .tf = self->tf,
        .nf = BIT(value, 15),
        .zf = (uint16_t)value == 0,
        .vf = BIT(value, 16) != BIT(value, 15),
        .cf = BIT(value, 17) != do_invert_cf,
    };
}
static inline void pdp11_ps_set_flags_from_xbyte(
    Pdp11Ps *const self,
    uint16_t const value,
    bool const do_invert_cf
) {
    *self = (Pdp11Ps){
        .priority = self->priority,
        .tf = self->tf,
        .nf = BIT(value, 7),
        .zf = (uint8_t)value == 0,
        .vf = BIT(value, 8) != BIT(value, 7),
        .cf = BIT(value, 9) != do_invert_cf,
    };
}

static uint16_t *pdp11_address_word(Pdp11 *const self, unsigned const mode) {
    unsigned const r_i = BITS(mode, 0, 2);

    unsigned const autostuff_amount = 2;
    switch (BITS(mode, 3, 5)) {
    case 00: return &pdp11_rx(self, r_i);
    case 01: return &pdp11_ram_word_at(self, pdp11_rx(self, r_i));
    case 02: {
        uint16_t *const result = &pdp11_ram_word_at(self, pdp11_rx(self, r_i));
        pdp11_rx(self, r_i) += autostuff_amount;
        return result;
    } break;
    case 03: {
        uint16_t *const result = &pdp11_ram_word_at(
            self,
            pdp11_ram_word_at(self, pdp11_rx(self, r_i))
        );
        pdp11_rx(self, r_i) += 2;
        return result;
    } break;
    case 04:
        return &pdp11_ram_word_at(
            self,
            pdp11_rx(self, r_i) -= autostuff_amount
        );
    case 05:
        return &pdp11_ram_word_at(
            self,
            pdp11_ram_word_at(self, pdp11_rx(self, r_i) -= 2)
        );
    case 06: {
        uint16_t const reg_val = pdp11_rx(self, r_i);
        return &pdp11_ram_word_at(self, reg_val + pdp11_instr_next(self));
    } break;
    case 07: {
        uint16_t const reg_val = pdp11_rx(self, r_i);
        return &pdp11_ram_word_at(
            self,
            pdp11_ram_word_at(self, reg_val + pdp11_instr_next(self))
        );
    } break;
    }

    return NULL;
}
static uint8_t *pdp11_address_byte(Pdp11 *const self, unsigned const mode) {
    unsigned const r_i = mode & 07;

    unsigned autostuff_amount = 1;
    switch (r_i) {
    case 06 ... 07:
        autostuff_amount = 2;
        /* fallthrough */
    case 00 ... 05:
        switch (BITS(mode, 3, 5)) {
        case 00: return &pdp11_rl(self, r_i);
        case 01: return &pdp11_ram_byte_at(self, pdp11_rx(self, r_i));
        case 02: {
            uint8_t *const result =
                &pdp11_ram_byte_at(self, pdp11_rx(self, r_i));
            pdp11_rx(self, r_i) += autostuff_amount;
            return result;
        } break;
        case 03: {
            uint8_t *const result = &pdp11_ram_byte_at(
                self,
                pdp11_ram_word_at(self, pdp11_rx(self, r_i))
            );
            pdp11_rx(self, r_i) += 2;
            return result;
        } break;
        case 04:
            return &pdp11_ram_byte_at(
                self,
                pdp11_rx(self, r_i) -= autostuff_amount
            );
        case 05:
            return &pdp11_ram_byte_at(
                self,
                pdp11_ram_word_at(self, pdp11_rx(self, r_i) -= 2)
            );
        case 06: {
            uint16_t const reg_val = pdp11_rx(self, r_i);
            return &pdp11_ram_byte_at(self, reg_val + pdp11_instr_next(self));
        } break;
        case 07: {
            uint16_t const reg_val = pdp11_rx(self, r_i);
            return &pdp11_ram_byte_at(
                self,
                pdp11_ram_word_at(self, reg_val + pdp11_instr_next(self))
            );
        } break;
        }
        /* fallthrough */
    default: return NULL;
    }
}

/************
 ** public **
 ************/

void pdp11_op_exec(Pdp11 *const self, uint16_t const instr) {
    uint16_t const opcode_15_12 = BITS(instr, 12, 15),
                   opcode_15_9 = BITS(instr, 9, 15),
                   opcode_15_6 = BITS(instr, 6, 15),
                   opcode_15_3 = BITS(instr, 3, 15),
                   opcode_15_0 = BITS(instr, 0, 15);

    unsigned const op_11_6 = BITS(instr, 6, 11), op_8_6 = BITS(instr, 6, 8),
                   op_5_0 = BITS(instr, 0, 5), op_8 = BIT(instr, 8),
                   op_7_0 = BITS(instr, 0, 7), op_2_0 = BITS(instr, 0, 2);

    switch (opcode_15_12) {
    case 001:
        return pdp11_op_mov(
            self,
            pdp11_address_word(self, op_11_6),
            pdp11_address_word(self, op_5_0)
        );
    case 011:
        return pdp11_op_movb(
            self,
            pdp11_address_byte(self, op_11_6),
            pdp11_address_byte(self, op_5_0)
        );
    case 002:
        return pdp11_op_cmp(
            self,
            pdp11_address_word(self, op_11_6),
            pdp11_address_word(self, op_5_0)
        );
    case 012:
        return pdp11_op_cmpb(
            self,
            pdp11_address_byte(self, op_11_6),
            pdp11_address_byte(self, op_5_0)
        );
    case 003:
        return pdp11_op_bit(
            self,
            pdp11_address_word(self, op_11_6),
            pdp11_address_word(self, op_5_0)
        );
    case 013:
        return pdp11_op_bitb(
            self,
            pdp11_address_byte(self, op_11_6),
            pdp11_address_byte(self, op_5_0)
        );
    case 004:
        return pdp11_op_bic(
            self,
            pdp11_address_word(self, op_11_6),
            pdp11_address_word(self, op_5_0)
        );
    case 014:
        return pdp11_op_bicb(
            self,
            pdp11_address_byte(self, op_11_6),
            pdp11_address_byte(self, op_5_0)
        );
    case 005:
        return pdp11_op_bis(
            self,
            pdp11_address_word(self, op_11_6),
            pdp11_address_word(self, op_5_0)
        );
    case 015:
        return pdp11_op_bisb(
            self,
            pdp11_address_byte(self, op_11_6),
            pdp11_address_byte(self, op_5_0)
        );
    case 006:
        return pdp11_op_add(
            self,
            pdp11_address_word(self, op_11_6),
            pdp11_address_word(self, op_5_0)
        );
    case 016:
        return pdp11_op_sub(
            self,
            pdp11_address_word(self, op_11_6),
            pdp11_address_word(self, op_5_0)
        );
    }
    switch (opcode_15_9) {
    case 0070:
        return pdp11_op_mul(self, op_8_6, pdp11_address_word(self, op_5_0));
    case 0071:
        return pdp11_op_div(self, op_8_6, pdp11_address_word(self, op_5_0));
    case 0072:
        return pdp11_op_ash(self, op_8_6, pdp11_address_word(self, op_5_0));
    case 0073:
        return pdp11_op_ashc(self, op_8_6, pdp11_address_word(self, op_5_0));
    case 0074:
        return pdp11_op_xor(self, op_8_6, pdp11_address_word(self, op_5_0));

    case 0000:
        if (op_8 != 1) break;
        return pdp11_op_br(self, op_7_0);
    case 0001: return pdp11_op_bne_be(self, op_8, op_7_0);
    case 0002: return pdp11_op_bge_bl(self, op_8, op_7_0);
    case 0003: return pdp11_op_bg_ble(self, op_8, op_7_0);
    case 0100: return pdp11_op_bpl_bmi(self, op_8, op_7_0);
    case 0101: return pdp11_op_bhi_blos(self, op_8, op_7_0);
    case 0102: return pdp11_op_bvc_bvs(self, op_8, op_7_0);
    case 0103: return pdp11_op_bcc_bcs(self, op_8, op_7_0);

    case 0077: return pdp11_op_sob(self, op_8_6, op_5_0);

    case 0004:
        return pdp11_op_jsr(self, op_8_6, pdp11_address_word(self, op_5_0));

    case 0104: return pdp11_op_emt_trap(self, op_8, op_7_0);
    }
    switch (opcode_15_6) {
    case 00001: return pdp11_op_jmp(self, pdp11_address_word(self, op_5_0));
    case 00003: return pdp11_op_swab(self, pdp11_address_word(self, op_5_0));
    case 00050: return pdp11_op_clr(self, pdp11_address_word(self, op_5_0));
    case 01050: return pdp11_op_clrb(self, pdp11_address_byte(self, op_5_0));
    case 00051: return pdp11_op_com(self, pdp11_address_word(self, op_5_0));
    case 01051: return pdp11_op_comb(self, pdp11_address_byte(self, op_5_0));
    case 00052: return pdp11_op_inc(self, pdp11_address_word(self, op_5_0));
    case 01052: return pdp11_op_incb(self, pdp11_address_byte(self, op_5_0));
    case 00053: return pdp11_op_dec(self, pdp11_address_word(self, op_5_0));
    case 01053: return pdp11_op_decb(self, pdp11_address_byte(self, op_5_0));
    case 00054: return pdp11_op_neg(self, pdp11_address_word(self, op_5_0));
    case 01054: return pdp11_op_negb(self, pdp11_address_byte(self, op_5_0));
    case 00055: return pdp11_op_adc(self, pdp11_address_word(self, op_5_0));
    case 01055: return pdp11_op_adcb(self, pdp11_address_byte(self, op_5_0));
    case 00056: return pdp11_op_sbc(self, pdp11_address_word(self, op_5_0));
    case 01056: return pdp11_op_sbcb(self, pdp11_address_byte(self, op_5_0));
    case 00057: return pdp11_op_tst(self, pdp11_address_word(self, op_5_0));
    case 01057: return pdp11_op_tstb(self, pdp11_address_byte(self, op_5_0));
    case 00060: return pdp11_op_ror(self, pdp11_address_word(self, op_5_0));
    case 01060: return pdp11_op_rorb(self, pdp11_address_byte(self, op_5_0));
    case 00061: return pdp11_op_rol(self, pdp11_address_word(self, op_5_0));
    case 01061: return pdp11_op_rolb(self, pdp11_address_byte(self, op_5_0));
    case 00062: return pdp11_op_asr(self, pdp11_address_word(self, op_5_0));
    case 01062: return pdp11_op_asrb(self, pdp11_address_byte(self, op_5_0));
    case 00063: return pdp11_op_asl(self, pdp11_address_word(self, op_5_0));
    case 01063: return pdp11_op_aslb(self, pdp11_address_byte(self, op_5_0));
    case 00065: return pdp11_op_mfpi(self, pdp11_address_word(self, op_5_0));
    case 01065: return pdp11_op_mfpd(self, pdp11_address_word(self, op_5_0));
    case 00066: return pdp11_op_mtpi(self, pdp11_address_word(self, op_5_0));
    case 01066: return pdp11_op_mtpd(self, pdp11_address_word(self, op_5_0));
    case 00067: return pdp11_op_sxt(self, pdp11_address_word(self, op_5_0));

    case 00064: return pdp11_op_mark(self, op_5_0);

    case 00002:
        if (!BIT(instr, 5)) break;
        return pdp11_op_clnzvc_senzvc(
            self,
            instr & (1 << 4),
            instr & (1 << 3),
            instr & (1 << 2),
            instr & (1 << 1),
            instr & (1 << 0)
        );
    }
    switch (opcode_15_3) {
    case 000020: return pdp11_op_rts(self, op_2_0);
    case 000023: return pdp11_op_spl(self, op_2_0);
    }
    switch (opcode_15_0) {
    case 0000002: return pdp11_op_rti(self);
    case 0000003: return pdp11_op_bpt(self);
    case 0000004: return pdp11_op_iot(self);
    case 0000006: return pdp11_op_rtt(self);

    case 0000000: return pdp11_op_halt(self);
    case 0000001: return pdp11_op_wait(self);
    case 0000005: return pdp11_op_reset(self);
    }

    // TODO ill
    abort();
}

/****************
 ** instr impl **
 ****************/

// SINGLE-OP

// general

void pdp11_op_clr(Pdp11 *const self, uint16_t *const dst) {
    *dst = 0;
    Pdp11Ps *const ps = &pdp11_ps(self);
    *ps = (Pdp11Ps){
        .priority = ps->priority,
        .tf = ps->tf,
        .nf = 0,
        .zf = 1,
        .vf = 0,
        .cf = 0,
    };
}
void pdp11_op_clrb(Pdp11 *const self, uint8_t *const dst) {
    *dst = 0;
    Pdp11Ps *const ps = &pdp11_ps(self);
    *ps = (Pdp11Ps){
        .priority = ps->priority,
        .tf = ps->tf,
        .nf = 0,
        .zf = 1,
        .vf = 0,
        .cf = 0,
    };
}
void pdp11_op_inc(Pdp11 *const self, uint16_t *const dst) {
    static uint16_t const src = 1;
    pdp11_op_add(self, &src, dst);
}
void pdp11_op_incb(Pdp11 *const self, uint8_t *const dst) {
    int16_t const result = (int16_t)*dst + 1;
    *dst = result;
    pdp11_ps_set_flags_from_xbyte(&pdp11_ps(self), result, false);
}
void pdp11_op_dec(Pdp11 *const self, uint16_t *const dst) {
    static uint16_t const src = -1;
    pdp11_op_sub(self, &src, dst);
}
void pdp11_op_decb(Pdp11 *const self, uint8_t *const dst) {
    int16_t const result = (int16_t)*dst - 1;
    *dst = result;
    pdp11_ps_set_flags_from_xbyte(&pdp11_ps(self), result, true);
}

void pdp11_op_neg(Pdp11 *const self, uint16_t *const dst) {
    int32_t const result = -(int32_t)*dst;
    *dst = result;
    pdp11_ps_set_flags_from_xword(&pdp11_ps(self), result, true);
}
void pdp11_op_negb(Pdp11 *const self, uint8_t *const dst) {
    int16_t const result = -(int16_t)*dst;
    *dst = result;
    pdp11_ps_set_flags_from_xbyte(&pdp11_ps(self), result, true);
}

void pdp11_op_tst(Pdp11 *const self, uint16_t const *const src) {
    static uint16_t const dst = 0;
    pdp11_op_cmp(self, src, &dst);
}
void pdp11_op_tstb(Pdp11 *const self, uint8_t const *const src) {
    static uint8_t const dst = 0;
    pdp11_op_cmpb(self, src, &dst);
}

void pdp11_op_com(Pdp11 *const self, uint16_t *const dst) {
    int32_t const result = ~(int32_t)*dst;
    *dst = result;
    pdp11_ps_set_flags_from_xword(&pdp11_ps(self), result, true);
}
void pdp11_op_comb(Pdp11 *const self, uint8_t *const dst) {
    int16_t const result = ~(int16_t)*dst;
    *dst = result;
    pdp11_ps_set_flags_from_xbyte(&pdp11_ps(self), result, true);
}

// shifts

void pdp11_op_asr(Pdp11 *const self, uint16_t *const dst) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_asrb(Pdp11 *const self, uint8_t *const dst) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_asl(Pdp11 *const self, uint16_t *const dst) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_aslb(Pdp11 *const self, uint8_t *const dst) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}

void pdp11_op_ash(Pdp11 *const self, unsigned const r_i, uint16_t *const dst) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_ashc(Pdp11 *const self, unsigned const r_i, uint16_t *const dst) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}

// multiple-percision

void pdp11_op_adc(Pdp11 *const self, uint16_t *const dst) {
    if (pdp11_ps(self).cf) pdp11_op_inc(self, dst);
}
void pdp11_op_adcb(Pdp11 *const self, uint8_t *const dst) {
    if (pdp11_ps(self).cf) pdp11_op_incb(self, dst);
}
void pdp11_op_sbc(Pdp11 *const self, uint16_t *const dst) {
    if (pdp11_ps(self).cf) pdp11_op_dec(self, dst);
}
void pdp11_op_sbcb(Pdp11 *const self, uint8_t *const dst) {
    if (pdp11_ps(self).cf) pdp11_op_decb(self, dst);
}

void pdp11_op_sxt(Pdp11 *const self, uint16_t *const dst) {
    Pdp11Ps *const ps = &pdp11_ps(self);
    *dst = -ps->nf;
    *ps = (Pdp11Ps){
        .priority = ps->priority,
        .tf = ps->tf,
        .nf = ps->nf,
        .zf = !ps->nf,
        .vf = 0,
        .cf = ps->cf,
    };
}

// rotates

// TODO badly implemented
void pdp11_op_ror(Pdp11 *const self, uint16_t *const dst) {
    Pdp11Ps *const ps = &pdp11_ps(self);

    uint32_t const dstx = (uint32_t)*dst | (pdp11_ps(self).cf << 16);
    ps->cf = BIT(dstx, 0);
    *dst = dstx >> 1;
    pdp11_ps_set_flags_from_word(ps, *dst);
    ps->vf = ps->nf ^ ps->cf;
}
void pdp11_op_rorb(Pdp11 *const self, uint8_t *const dst) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_rol(Pdp11 *const self, uint16_t *const dst) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_rolb(Pdp11 *const self, uint8_t *const dst) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}

void pdp11_op_swab(Pdp11 *const self, uint16_t *const dst) {
    *dst = (uint16_t)(*dst << 8) | (uint8_t)(*dst >> 8);
    pdp11_ps_set_flags_from_word(&pdp11_ps(self), *dst);
}

// DUAL-OP

// arithmetic

void pdp11_op_mov(
    Pdp11 *const self,
    uint16_t const *const src,
    uint16_t *const dst
) {
    *dst = *src;
    pdp11_ps_set_flags_from_word(&pdp11_ps(self), *src);
}
void pdp11_op_movb(
    Pdp11 *const self,
    uint8_t const *const src,
    uint8_t *const dst
) {
    uint16_t *const dst16 = (uint16_t *)dst;
    if ((uintptr_t)dst16 % alignof(uint16_t) == 0 &&
        &pdp11_rx(self, 0) <= dst16 &&
        dst16 <= &pdp11_rx(self, PDP11_REGISTER_COUNT - 1)) {
        *dst16 = (int16_t)(int8_t)*src;
    } else {
        *dst = *src;
    }
    pdp11_ps_set_flags_from_byte(&pdp11_ps(self), *dst);
}
void pdp11_op_add(
    Pdp11 *const self,
    uint16_t const *const src,
    uint16_t *const dst
) {
    uint32_t const result = xword(*src) + xword(*dst);
    *dst = result;
    pdp11_ps_set_flags_from_xword(&pdp11_ps(self), result, false);
}
void pdp11_op_sub(
    Pdp11 *const self,
    uint16_t const *const src,
    uint16_t *const dst
) {
    uint32_t const result = xword(*src) + xword(-*dst);
    *dst = result;
    pdp11_ps_set_flags_from_xword(&pdp11_ps(self), result, true);
}
void pdp11_op_cmp(
    Pdp11 *const self,
    uint16_t const *const src,
    uint16_t const *const dst
) {
    pdp11_ps_set_flags_from_xword(
        &pdp11_ps(self),
        xword(*src) + xword(-*dst),
        true
    );
}
void pdp11_op_cmpb(
    Pdp11 *const self,
    uint8_t const *const src,
    uint8_t const *const dst
) {
    pdp11_ps_set_flags_from_xbyte(
        &pdp11_ps(self),
        xbyte(*src) + xbyte(-*dst),
        true
    );
}

// register destination

void pdp11_op_mul(
    Pdp11 *const self,
    unsigned const r_i,
    uint16_t const *const src
) {
    Pdp11Ps *const ps = &pdp11_ps(self);

    uint32_t const result =
        (uint32_t)((int16_t)*src * (int16_t)pdp11_rx(self, r_i));
    if ((r_i & 1) == 0) pdp11_rx(self, r_i | 1) = (uint16_t)(result >> 16);
    pdp11_rx(self, r_i) = (uint16_t)result;

    *ps = (Pdp11Ps){
        .priority = ps->priority,
        .tf = ps->tf,
        .nf = BIT(result, 15),
        .zf = (uint16_t)result == 0,
        .vf = 0,
        .cf = (uint16_t)(result >> 16) != (BIT(result, 15) ? 0xFFFF : 0x0000),
    };
}
void pdp11_op_div(
    Pdp11 *const self,
    unsigned const r_i,
    uint16_t const *const src
) {
    Pdp11Ps *const ps = &pdp11_ps(self);

    if (*src == 0) {
        ps->vf = ps->cf = 1;
        return;
    }

    uint32_t dst_value = pdp11_rx(self, r_i);
    if ((r_i & 1) == 0) dst_value |= pdp11_rx(self, r_i | 1) << 16;

    uint32_t const quotient = dst_value / *src;

    if ((uint16_t)(quotient >> 16) != 0) {
        ps->vf = 1;
        return;
    }

    pdp11_rx(self, r_i) = quotient;
    if ((r_i & 1) == 0) pdp11_rx(self, r_i) = dst_value % *src;

    *ps = (Pdp11Ps){
        .priority = ps->priority,
        .tf = ps->tf,
        .nf = BIT(quotient, 15),
        .zf = quotient == 0,
        .vf = 0,
        .cf = 0,
    };
}

void pdp11_op_xor(Pdp11 *const self, unsigned const r_i, uint16_t *const dst) {
    *dst ^= pdp11_rx(self, r_i);
    pdp11_ps_set_flags_from_word(&pdp11_ps(self), *dst);
}

// logical

void pdp11_op_bit(
    Pdp11 *const self,
    uint16_t const *const src,
    uint16_t const *const dst
) {
    pdp11_ps_set_flags_from_word(&pdp11_ps(self), *dst & *src);
}
void pdp11_op_bitb(
    Pdp11 *const self,
    uint8_t const *const src,
    uint8_t const *const dst
) {
    pdp11_ps_set_flags_from_byte(&pdp11_ps(self), *dst & *src);
}
void pdp11_op_bis(
    Pdp11 *const self,
    uint16_t const *const src,
    uint16_t *const dst
) {
    *dst |= *src;
    pdp11_op_bit(self, src, dst);
}
void pdp11_op_bisb(
    Pdp11 *const self,
    uint8_t const *const src,
    uint8_t *const dst
) {
    *dst |= *src;
    pdp11_op_bitb(self, src, dst);
}
void pdp11_op_bic(
    Pdp11 *const self,
    uint16_t const *const src,
    uint16_t *const dst
) {
    *dst &= ~*src;
    pdp11_op_bit(self, src, dst);
}
void pdp11_op_bicb(
    Pdp11 *const self,
    uint8_t const *const src,
    uint8_t *const dst
) {
    *dst &= ~*src;
    pdp11_op_bitb(self, src, dst);
}

// PROGRAM CONTROL

// branches

void pdp11_op_br(Pdp11 *const self, uint8_t const off) {
    pdp11_pc(self) += 2 * (int16_t)(int8_t)off;
}

void pdp11_op_bne_be(Pdp11 *const self, bool const cond, uint8_t const off) {
    if (pdp11_ps(self).zf == cond) pdp11_op_br(self, off);
}
void pdp11_op_bpl_bmi(Pdp11 *const self, bool const cond, uint8_t const off) {
    if (pdp11_ps(self).nf == cond) pdp11_op_br(self, off);
}
void pdp11_op_bcc_bcs(Pdp11 *const self, bool const cond, uint8_t const off) {
    if (pdp11_ps(self).cf == cond) pdp11_op_br(self, off);
}
void pdp11_op_bvc_bvs(Pdp11 *const self, bool const cond, uint8_t const off) {
    if (pdp11_ps(self).vf == cond) pdp11_op_br(self, off);
}

void pdp11_op_bge_bl(Pdp11 *const self, bool const cond, uint8_t const off) {
    Pdp11Ps *const ps = &pdp11_ps(self);
    if ((ps->nf ^ ps->vf) == cond) pdp11_op_br(self, off);
}
void pdp11_op_bg_ble(Pdp11 *const self, bool const cond, uint8_t const off) {
    Pdp11Ps *const ps = &pdp11_ps(self);
    if ((ps->zf | (ps->nf ^ ps->vf)) == cond) pdp11_op_br(self, off);
}

void pdp11_op_bhi_blos(Pdp11 *const self, bool const cond, uint8_t const off) {
    Pdp11Ps *const ps = &pdp11_ps(self);
    if ((ps->cf | ps->zf) == cond) pdp11_op_br(self, off);
}

// subroutine

void pdp11_op_jsr(
    Pdp11 *const self,
    unsigned const r_i,
    uint16_t const *const src
) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_mark(Pdp11 *const self, unsigned const param_count) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_rts(Pdp11 *const self, unsigned const r_i) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}

// program control

void pdp11_op_spl(Pdp11 *const self, unsigned const value) {
    pdp11_ps(self).priority = value;
}

void pdp11_op_jmp(Pdp11 *const self, uint16_t const *const src) {
    pdp11_pc(self) = *src;
}
void pdp11_op_sob(Pdp11 *const self, unsigned const r_i, uint8_t const off) {
    if (--pdp11_rx(self, r_i) != 0) pdp11_pc(self) -= 2 * off;
}

// trap

void pdp11_op_emt_trap(
    Pdp11 *const self,
    bool const is_trap,
    uint8_t const code
) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_bpt(Pdp11 *const self) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_iot(Pdp11 *const self) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_rti(Pdp11 *const self) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_rtt(Pdp11 *const self) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}

// MISC.

void pdp11_op_halt(Pdp11 *const self) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_wait(Pdp11 *const self) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_reset(Pdp11 *const self) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
// NOTE `nop` is already implemented with `clnzvc`/`senzvc`

void pdp11_op_mtpd(Pdp11 *const self, uint16_t *const dst) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_mtpi(Pdp11 *const self, uint16_t *const dst) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_mfpd(Pdp11 *const self, uint16_t const *const src) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_mfpi(Pdp11 *const self, uint16_t const *const src) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}

// CONDITION CODES

void pdp11_op_clnzvc_senzvc(
    Pdp11 *const self,
    bool const value,
    bool const do_affect_nf,
    bool const do_affect_zf,
    bool const do_affect_vf,
    bool const do_affect_cf
) {
    if (do_affect_nf) pdp11_ps(self).nf = value;
    if (do_affect_zf) pdp11_ps(self).zf = value;
    if (do_affect_vf) pdp11_ps(self).vf = value;
    if (do_affect_cf) pdp11_ps(self).cf = value;
}
