#include "pdp11/pdp11_op.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/****************
 ** instr decl **
 ****************/

static alwaysinline void
pdp11_op_mov(Pdp11 *const self, uint16_t const *const src, uint16_t *const dst);
static alwaysinline void
pdp11_op_movb(Pdp11 *const self, uint8_t const *const src, uint8_t *const dst);

static alwaysinline void pdp11_op_cmp(
    Pdp11 *const self,
    uint16_t const *const src,
    uint16_t const *const dst
);
static alwaysinline void pdp11_op_cmpb(
    Pdp11 *const self,
    uint8_t const *const src,
    uint8_t const *const dst
);

static alwaysinline void
pdp11_op_xor(Pdp11 *const self, unsigned const r_i, uint16_t *const dst);

static alwaysinline void pdp11_op_ccc_scc(
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
pdp11_ps_set_flags_from(Pdp11Ps *const self, int32_t const value) {
    *self = (Pdp11Ps){
        .priority = self->priority,
        // TODO? skipped tflag
        .nf = value < 0,
        .zf = value == 0,
        .vf = ((value >> 31) & 1) != ((value >> 15) & 1),
        .cf = ((value >> 31) & 1) != ((value >> 16) & 1),
    };
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
        case 00: return self->cpu.r + r_i;
        case 01: return &pdp11_ram_byte_at(self, self->cpu.r[r_i]);
        case 02: {
            uint8_t *const result = &pdp11_ram_byte_at(self, self->cpu.r[r_i]);
            self->cpu.r[r_i] += autodecrement_amount;
            return result;
        } break;
        case 03: {
            uint8_t *const result = &pdp11_ram_byte_at(
                self,
                pdp11_ram_word_at(self, self->cpu.r[r_i])
            );
            self->cpu.r[r_i] += 2;
            return result;
        } break;
        case 04:
            return &pdp11_ram_byte_at(
                self,
                self->cpu.r[r_i] -= autodecrement_amount
            );
        case 05:
            return &pdp11_ram_byte_at(
                self,
                pdp11_ram_word_at(self, self->cpu.r[r_i] -= 2)
            );
        case 06: {
            uint16_t const reg_val = self->cpu.r[r_i];
            return &pdp11_ram_byte_at(self, reg_val + pdp11_instr_next(self));
        } break;
        case 07: {
            uint16_t const reg_val = self->cpu.r[r_i];
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

    case 0000000: return;  // halt
    case 0000001: return;  // wait
    case 0000005: return;  // reset
    }

    // ill
    abort();
}

/****************
 ** instr impl **
 ****************/

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
    *dst = *src;

    if ((uintptr_t)dst % alignof(uint16_t) == 0) {
        uint16_t *const dst16 = (uint16_t *)dst;
        if (self->cpu.r <= dst16 && dst16 < self->cpu.r + PDP11_REGISTER_COUNT)
            *(dst + 1) = *dst & (1 << 7) ? 0xFF : 0x00;
    }
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
    pdp11_ps_set_flags_from(&self->cpu.ps, (int32_t)*src - (int32_t)*dst);
}
void pdp11_op_cmpb(
    Pdp11 *const self,
    uint8_t const *const src,
    uint8_t const *const dst
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
    pdp11_ps_set_flags_from(&self->cpu.ps, (int32_t)*src - (int32_t)*dst);
}

void pdp11_op_xor(Pdp11 *const self, unsigned const r_i, uint16_t *const dst) {
    *dst = *dst ^ self->cpu.r[r_i];
}

void pdp11_op_ccc_scc(
    Pdp11 *const self,
    bool const value,
    bool const do_affect_nf,
    bool const do_affect_zf,
    bool const do_affect_vf,
    bool const do_affect_cf
) {
    if (do_affect_nf) self->cpu.ps.nf = value;
    if (do_affect_zf) self->cpu.ps.zf = value;
    if (do_affect_vf) self->cpu.ps.vf = value;
    if (do_affect_cf) self->cpu.ps.cf = value;
}
