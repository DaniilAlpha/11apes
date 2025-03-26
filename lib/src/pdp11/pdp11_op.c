#include "pdp11/pdp11_op.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/****************
 ** instr decl **
 ****************/

// dual-op

static forceinline void
pdp11_op_mov(Pdp11 *const self, uint16_t const *const src, uint16_t *const dst);
static forceinline void
pdp11_op_movb(Pdp11 *const self, uint8_t const *const src, uint8_t *const dst);

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
pdp11_op_bic(Pdp11 *const self, uint16_t const *const src, uint16_t *const dst);
static forceinline void
pdp11_op_bicb(Pdp11 *const self, uint8_t const *const src, uint8_t *const dst);

static forceinline void
pdp11_op_bis(Pdp11 *const self, uint16_t const *const src, uint16_t *const dst);
static forceinline void
pdp11_op_bisb(Pdp11 *const self, uint8_t const *const src, uint8_t *const dst);

static forceinline void
pdp11_op_add(Pdp11 *const self, uint16_t const *const src, uint16_t *const dst);
static forceinline void
pdp11_op_sub(Pdp11 *const self, uint8_t const *const src, uint8_t *const dst);

// limited dual-op

static forceinline void
pdp11_op_mul(Pdp11 *const self, unsigned const r_i, uint16_t const *const src);
static forceinline void
pdp11_op_div(Pdp11 *const self, unsigned const r_i, uint16_t const *const src);

static forceinline void
pdp11_op_ash(Pdp11 *const self, unsigned const r_i, uint16_t const *const src);
static forceinline void
pdp11_op_ashc(Pdp11 *const self, unsigned const r_i, uint16_t const *const src);

static forceinline void
pdp11_op_xor(Pdp11 *const self, unsigned const r_i, uint16_t *const dst);

// single-op

static forceinline void
pdp11_op_jmp(Pdp11 *const self, uint16_t const *const src);

static forceinline void pdp11_op_swab(Pdp11 *const self, uint16_t *const dst);

static forceinline void pdp11_op_clr(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_clrb(Pdp11 *const self, uint8_t *const dst);

static forceinline void pdp11_op_com(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_comb(Pdp11 *const self, uint8_t *const dst);

static forceinline void pdp11_op_inc(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_incb(Pdp11 *const self, uint8_t *const dst);
static forceinline void pdp11_op_dec(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_decb(Pdp11 *const self, uint8_t *const dst);

static forceinline void pdp11_op_neg(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_negb(Pdp11 *const self, uint8_t *const dst);

static forceinline void pdp11_op_adc(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_adcb(Pdp11 *const self, uint8_t *const dst);
static forceinline void pdp11_op_sbc(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_sbcb(Pdp11 *const self, uint8_t *const dst);

static forceinline void
pdp11_op_tst(Pdp11 *const self, uint16_t const *const src);
static forceinline void
pdp11_op_tstb(Pdp11 *const self, uint8_t const *const src);

static forceinline void pdp11_op_ror(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_rorb(Pdp11 *const self, uint8_t *const dst);
static forceinline void pdp11_op_rol(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_rolb(Pdp11 *const self, uint8_t *const dst);

static forceinline void pdp11_op_asr(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_asrb(Pdp11 *const self, uint8_t *const dst);
static forceinline void pdp11_op_asl(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_aslb(Pdp11 *const self, uint8_t *const dst);

static forceinline void
pdp11_op_mtps(Pdp11 *const self, uint16_t const *const src);
static forceinline void pdp11_op_mfps(Pdp11 *const self, uint16_t *const dst);

static forceinline void
pdp11_op_mfpi(Pdp11 *const self, uint16_t const *const src);
static forceinline void
pdp11_op_mfpd(Pdp11 *const self, uint16_t const *const src);
static forceinline void pdp11_op_mtpi(Pdp11 *const self, uint16_t *const dst);
static forceinline void pdp11_op_mtpd(Pdp11 *const self, uint16_t *const dst);

// TODO! have no idea if this should be a byte or a word
static forceinline void pdp11_op_sxt(Pdp11 *const self, uint16_t *const dst);

// branch

static forceinline void pdp11_op_br(Pdp11 *const self, uint8_t const off);

static forceinline void
pdp11_op_bne_be(Pdp11 *const self, bool const cond, uint8_t const off);
static forceinline void
pdp11_op_bge_bl(Pdp11 *const self, bool const cond, uint8_t const off);
static forceinline void
pdp11_op_bg_ble(Pdp11 *const self, bool const cond, uint8_t const off);

static forceinline void
pdp11_op_bpl_bmi(Pdp11 *const self, bool const cond, uint8_t const off);

static forceinline void
pdp11_op_bhi_blos(Pdp11 *const self, bool const cond, uint8_t const off);
static forceinline void
pdp11_op_bhis_blo(Pdp11 *const self, bool const cond, uint8_t const off);

static forceinline void
pdp11_op_bvc_bvs(Pdp11 *const self, bool const cond, uint8_t const off);

static forceinline void
pdp11_op_sob(Pdp11 *const self, unsigned const r_i, uint8_t const off);

// subroutine

static forceinline void
pdp11_op_jsr(Pdp11 *const self, uint16_t *const src, unsigned const r_i);
static forceinline void pdp11_op_rts(Pdp11 *const self, unsigned const r_i);
// TODO? WTF is nn
static forceinline void pdp11_op_mark(Pdp11 *const self, uint8_t const nn);

// trap

static forceinline void
pdp11_op_emt_trap(Pdp11 *const self, bool const is_general, uint8_t const code);
static forceinline void pdp11_op_bpt(Pdp11 *const self);
static forceinline void pdp11_op_iot(Pdp11 *const self);

static forceinline void pdp11_op_rti(Pdp11 *const self);
static forceinline void pdp11_op_rtt(Pdp11 *const self);

// misc.

static forceinline void pdp11_op_halt(Pdp11 *const self);
static forceinline void pdp11_op_wait(Pdp11 *const self);
static forceinline void pdp11_op_reset(Pdp11 *const self);
// nop is already implemented with ccc/scc

// ccodes

static forceinline void pdp11_op_ccc_scc(
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
pdp11_ps_test_for_flags(Pdp11Ps *const self, int32_t const value) {
    *self = (Pdp11Ps){
        .priority = self->priority,
        .tf = self->tf,
        .nf = value < 0,
        .zf = value == 0,
        .vf = ((value >> 31) & 1) != ((value >> 15) & 1),
        .cf = ((value >> 31) & 1) != ((value >> 16) & 1),
    };
}

static inline Pdp11Ps pdp11_ps_from_word(uint16_t const value) {
    return (Pdp11Ps){
        .priority = value >> 5,
        .tf = value >> 4,
        .nf = value >> 3,
        .zf = value >> 2,
        .vf = value >> 1,
        .cf = value >> 0,
    };
}
static inline uint16_t pdp11_ps_to_word(Pdp11Ps *const self) {
    return self->priority << 5 | self->tf << 4 | self->nf << 3 | self->zf << 2 |
           self->vf << 1 | self->cf << 0;
}

// TODO? probably need two separate functions for words and bytes
static void *pdp11_address(
    Pdp11 *const self,
    unsigned const mode,
    bool const do_autodecrement_word
) {
    unsigned const r_i = mode & 07;

    unsigned autodecrement_amount = do_autodecrement_word ? 2 : 1;
    switch (r_i) {
    case 06 ... 07:
        autodecrement_amount = 2;
        /* fallthrough */
    case 00 ... 05:
        switch ((mode >> 3) & 07) {
        case 00: return &pdp11_rx(self, r_i);
        case 01: return &pdp11_ram_byte_at(self, pdp11_rx(self, r_i));
        case 02: {
            uint8_t *const result =
                &pdp11_ram_byte_at(self, pdp11_rx(self, r_i));
            pdp11_rx(self, r_i) += autodecrement_amount;
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
                pdp11_rx(self, r_i) -= autodecrement_amount
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
    uint16_t const opcode_15_12 = (instr >> 12) & 017,
                   opcode_15_9 = (instr >> 9) & 0177,
                   opcode_15_6 = (instr >> 6) & 01777,
                   opcode_15_3 = (instr >> 3) & 017777,
                   opcode_15_0 = (instr >> 0) & 0177777;

    printf(
        "executing : 0%06o, (0%02o 0%03o 0%04o 0%05o 0%06o)\n",
        instr,
        opcode_15_12,
        opcode_15_9,
        opcode_15_6,
        opcode_15_3,
        opcode_15_0
    );
    switch (opcode_15_12) {
    case 001:
        return pdp11_op_mov(
            self,
            pdp11_address(self, (instr >> 6) & 077, true),
            pdp11_address(self, (instr >> 0) & 077, true)
        );
    case 011:
        return pdp11_op_movb(
            self,
            pdp11_address(self, (instr >> 6) & 077, false),
            pdp11_address(self, (instr >> 0) & 077, false)
        );
    case 002:
        return pdp11_op_cmp(
            self,
            pdp11_address(self, (instr >> 6) & 077, true),
            pdp11_address(self, (instr >> 0) & 077, true)
        );
    case 012:
        return pdp11_op_cmpb(
            self,
            pdp11_address(self, (instr >> 6) & 077, false),
            pdp11_address(self, (instr >> 0) & 077, false)
        );
    case 003: return;  // bit
    case 013: return;  // bitb
    case 004: return;  // bic
    case 014: return;  // bicb
    case 005: return;  // bis
    case 015: return;  // bisb
    case 006: return;  // add
    case 016: return;  // sub
    }
    switch (opcode_15_9) {
    case 0070: return;  // mul
    case 0071: return;  // div
    case 0072: return;  // ash
    case 0073: return;  // ashc
    case 0074:
        return pdp11_op_xor(
            self,
            (instr >> 6) & 07,
            pdp11_address(self, (instr >> 0) & 077, true)
        );

    case 0000:
        if (!(instr & (1 >> 8))) break;
        return;         // br
    case 0001: return;  // bne/beq
    case 0002: return;  // bge/blt
    case 0003: return;  // bge/blt
    case 0100: return;  // bpl/bmi
    case 0101: return;  // bhi/blos
    case 0102: return;  // bvc/bvs
    case 0103: return;  // bhis/blo

    case 0077: return;  // sob

    case 0004: return;  // jsr

    case 0104: return;  // emt/trap
    }
    switch (opcode_15_6) {
    case 00001: return;  // jmp
    case 00003: return;  // swab
    case 00050: return;  // clr
    case 01050: return;  // clrb
    case 00051: return;  // com
    case 01051: return;  // comb
    case 00052: return;  // inc
    case 01052: return;  // incb
    case 00053: return;  // dec
    case 01053: return;  // decb
    case 00054: return;  // neg
    case 01054: return;  // negb
    case 00055: return;  // adc
    case 01055: return;  // adcb
    case 00056: return;  // sbc
    case 01056: return;  // sbcb
    case 00057: return;  // tst
    case 01057: return;  // tstb
    case 00060: return;  // ror
    case 01060: return;  // rorb
    case 00061: return;  // rol
    case 01061: return;  // rolb
    case 00062: return;  // asr
    case 01062: return;  // asrb
    case 00063: return;  // asl
    case 01063: return;  // aslb
    case 01064: return;  // mtps
    case 00065: return;  // mfpi
    case 01065: return;  // mfpd
    case 00066: return;  // mtpi
    case 01066: return;  // mtpd
    case 00067: return;  // sxt
    case 01067: return;  // mfps

    case 00064: return;  // mark

    case 00002:
        if (!(instr & (1 << 5))) break;
        return pdp11_op_ccc_scc(
            self,
            instr & (1 << 4),
            instr & (1 << 3),
            instr & (1 << 2),
            instr & (1 << 1),
            instr & (1 << 0)
        );
    }
    switch (opcode_15_3) {
    case 000020: return;  // rts
    }
    switch (opcode_15_0) {
    case 0000002: return;  // rti
    case 0000003: return;  // bpt
    case 0000004: return;  // iot
    case 0000006: return;  // rtt

    case 0000000: return pdp11_op_halt(self);
    case 0000001: return pdp11_op_wait(self);
    case 0000005: return pdp11_op_reset(self);
    }

    // ill
    abort();
}

/****************
 ** instr impl **
 ****************/

// dual-op

void pdp11_op_mov(
    Pdp11 *const self,
    uint16_t const *const src,
    uint16_t *const dst
) {
    *dst = *src;
}
void pdp11_op_movb(
    Pdp11 *const self,
    uint8_t const *const src,
    uint8_t *const dst
) {
    uint16_t *const dst16 = (uint16_t *)dst;
    if ((uintptr_t)dst16 % alignof(uint16_t) == 0 &&
        &pdp11_rx(self, 0) <= dst16 &&
        dst16 <= &pdp11_rx(self, PDP11_REGISTER_COUNT - 1))
        *dst16 = (int16_t)(int8_t)*src;
    else
        *dst = *src;
}

void pdp11_op_cmp(
    Pdp11 *const self,
    uint16_t const *const src,
    uint16_t const *const dst
) {
    /* uint64_t flags;
    asm inline(
        "\t\n   cmp %[dst], %[src]"
        "\t\n   pushf"
        "\t\n   pop %[out_flags]"
        : [out_flags] "=r"(flags)
        : [src] "r"(*src), [dst] "r"(*dst)
        : "cc"
    );
    pdp11_ps_of_x86(&self->cpu.ps, flags); */
    pdp11_ps_test_for_flags(&pdp11_ps(self), (int32_t)*src - (int32_t)*dst);
}
void pdp11_op_cmpb(
    Pdp11 *const self,
    uint8_t const *const src,
    uint8_t const *const dst
) {
    pdp11_ps_test_for_flags(&pdp11_ps(self), (int16_t)*src - (int16_t)*dst);
}

void pdp11_op_bit(
    Pdp11 *const self,
    uint16_t const *const src,
    uint16_t const *const dst
) {
    // TODO? should it be implemented this way?
    pdp11_ps_test_for_flags(&pdp11_ps(self), (int32_t)*src ^ (int32_t)*dst);
}
void pdp11_op_bitb(
    Pdp11 *const self,
    uint8_t const *const src,
    uint8_t const *const dst
) {
    pdp11_ps_test_for_flags(&pdp11_ps(self), (int16_t)*src ^ (int16_t)*dst);
}

void pdp11_op_bic(
    Pdp11 *const self,
    uint16_t const *const src,
    uint16_t *const dst
) {
    *dst &= ~*src;
}
void pdp11_op_bicb(
    Pdp11 *const self,
    uint8_t const *const src,
    uint8_t *const dst
) {
    *dst &= ~*src;
}

void pdp11_op_bis(
    Pdp11 *const self,
    uint16_t const *const src,
    uint16_t *const dst
) {
    *dst |= *src;
}
void pdp11_op_bisb(
    Pdp11 *const self,
    uint8_t const *const src,
    uint8_t *const dst
) {
    *dst |= *src;
}

void pdp11_op_add(
    Pdp11 *const self,
    uint16_t const *const src,
    uint16_t *const dst
) {
    *dst += *src;
}
void pdp11_op_sub(
    Pdp11 *const self,
    uint8_t const *const src,
    uint8_t *const dst
) {
    *dst -= *src;
}

// limited dual-op

void pdp11_op_mul(
    Pdp11 *const self,
    unsigned const r_i,
    uint16_t const *const src
);
void pdp11_op_div(
    Pdp11 *const self,
    unsigned const r_i,
    uint16_t const *const src
);

void pdp11_op_ash(
    Pdp11 *const self,
    unsigned const r_i,
    uint16_t const *const src
);
void pdp11_op_ashc(
    Pdp11 *const self,
    unsigned const r_i,
    uint16_t const *const src
);

void pdp11_op_xor(Pdp11 *const self, unsigned const r_i, uint16_t *const dst) {
    *dst = *dst ^ pdp11_rx(self, r_i);
}

// single-op

void pdp11_op_jmp(Pdp11 *const self, uint16_t const *const src) {
    pdp11_pc(self) = *src;
}

void pdp11_op_swab(Pdp11 *const self, uint16_t *const dst) {
    *dst = (uint16_t)(*dst << 8) | (uint8_t)(*dst >> 8);
}

void pdp11_op_clr(Pdp11 *const self, uint16_t *const dst) { *dst = 0; }
void pdp11_op_clrb(Pdp11 *const self, uint8_t *const dst) { *dst = 0; }

void pdp11_op_com(Pdp11 *const self, uint16_t *const dst) { *dst = ~*dst; }
void pdp11_op_comb(Pdp11 *const self, uint8_t *const dst) { *dst = ~*dst; }

void pdp11_op_inc(Pdp11 *const self, uint16_t *const dst) { (*dst)++; }
void pdp11_op_incb(Pdp11 *const self, uint8_t *const dst) { (*dst)++; }
void pdp11_op_dec(Pdp11 *const self, uint16_t *const dst) { (*dst)--; }
void pdp11_op_decb(Pdp11 *const self, uint8_t *const dst) { (*dst)--; }

void pdp11_op_neg(Pdp11 *const self, uint16_t *const dst) { *dst = -*dst; }
void pdp11_op_negb(Pdp11 *const self, uint8_t *const dst) { *dst = -*dst; }

void pdp11_op_adc(Pdp11 *const self, uint16_t *const dst) {
    *dst += pdp11_ps(self).cf;
}
void pdp11_op_adcb(Pdp11 *const self, uint8_t *const dst) {
    *dst += pdp11_ps(self).cf;
}
void pdp11_op_sbc(Pdp11 *const self, uint16_t *const dst) {
    *dst -= pdp11_ps(self).cf;
}
void pdp11_op_sbcb(Pdp11 *const self, uint8_t *const dst) {
    *dst -= pdp11_ps(self).cf;
}

void pdp11_op_tst(Pdp11 *const self, uint16_t const *const src) {
    pdp11_ps_test_for_flags(&pdp11_ps(self), (int32_t)*src);
}
void pdp11_op_tstb(Pdp11 *const self, uint8_t const *const src) {
    // TODO? is this so?
    pdp11_ps_test_for_flags(&pdp11_ps(self), (int16_t)*src);
}

void pdp11_op_ror(Pdp11 *const self, uint16_t *const dst);
void pdp11_op_rorb(Pdp11 *const self, uint8_t *const dst);
void pdp11_op_rol(Pdp11 *const self, uint16_t *const dst);
void pdp11_op_rolb(Pdp11 *const self, uint8_t *const dst);

void pdp11_op_asr(Pdp11 *const self, uint16_t *const dst) { *dst >>= 1; }
void pdp11_op_asrb(Pdp11 *const self, uint8_t *const dst) { *dst >>= 1; }
void pdp11_op_asl(Pdp11 *const self, uint16_t *const dst) { *dst <<= 1; }
void pdp11_op_aslb(Pdp11 *const self, uint8_t *const dst) { *dst <<= 1; }

void pdp11_op_mtps(Pdp11 *const self, uint16_t const *const src) {
    pdp11_ps(self) = pdp11_ps_from_word(*src);
}
void pdp11_op_mfps(Pdp11 *const self, uint16_t *const dst) {
    *dst = pdp11_ps_to_word(&pdp11_ps(self));
}

void pdp11_op_mfpi(Pdp11 *const self, uint16_t const *const src);
void pdp11_op_mfpd(Pdp11 *const self, uint16_t const *const src);
void pdp11_op_mtpi(Pdp11 *const self, uint16_t *const dst);
void pdp11_op_mtpd(Pdp11 *const self, uint16_t *const dst);

void pdp11_op_sxt(Pdp11 *const self, uint16_t *const dst);

// branch

void pdp11_op_br(Pdp11 *const self, uint8_t const off);

void pdp11_op_bne_be(Pdp11 *const self, bool const cond, uint8_t const off);
void pdp11_op_bge_bl(Pdp11 *const self, bool const cond, uint8_t const off);
void pdp11_op_bg_ble(Pdp11 *const self, bool const cond, uint8_t const off);

void pdp11_op_bpl_bmi(Pdp11 *const self, bool const cond, uint8_t const off);

void pdp11_op_bhi_blos(Pdp11 *const self, bool const cond, uint8_t const off);
void pdp11_op_bhis_blo(Pdp11 *const self, bool const cond, uint8_t const off);

void pdp11_op_bvc_bvs(Pdp11 *const self, bool const cond, uint8_t const off);

void pdp11_op_sob(Pdp11 *const self, unsigned const r_i, uint8_t const off);

// subroutine

void pdp11_op_jsr(Pdp11 *const self, uint16_t *const src, unsigned const r_i);
void pdp11_op_rts(Pdp11 *const self, unsigned const r_i);
// TODO? WTF is nn
void pdp11_op_mark(Pdp11 *const self, uint8_t const nn);

// trap

void pdp11_op_emt_trap(
    Pdp11 *const self,
    bool const is_general,
    uint8_t const code
);
void pdp11_op_bpt(Pdp11 *const self);
void pdp11_op_iot(Pdp11 *const self);

void pdp11_op_rti(Pdp11 *const self);
void pdp11_op_rtt(Pdp11 *const self);

// misc.

void pdp11_op_halt(Pdp11 *const self);
void pdp11_op_wait(Pdp11 *const self);
void pdp11_op_reset(Pdp11 *const self);
// nop is already implemented with ccc/scc

// ccodes

void pdp11_op_ccc_scc(
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
