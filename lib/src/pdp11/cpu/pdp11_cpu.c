#include "pdp11/cpu/pdp11_cpu.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdio.h>

#include "conviniences.h"

// TODO! Both JMP and JSR, used in address mode 2 (autoincrement), increment
// the register before using it as an address. This is a special case. and is
// not true of any other instruction

// TODO properly implement memory errors with traps: stack overflow, bus error,
// and also illegal instructions

/****************
 ** instr decl **
 ****************/

// SINGLE-OP

// general

forceinline void pdp11_cpu_instr_clr(Pdp11Cpu *const self, uint16_t *const dst);
forceinline void pdp11_cpu_instr_clrb(Pdp11Cpu *const self, uint8_t *const dst);
forceinline void pdp11_cpu_instr_inc(Pdp11Cpu *const self, uint16_t *const dst);
forceinline void pdp11_cpu_instr_incb(Pdp11Cpu *const self, uint8_t *const dst);
forceinline void pdp11_cpu_instr_dec(Pdp11Cpu *const self, uint16_t *const dst);
forceinline void pdp11_cpu_instr_decb(Pdp11Cpu *const self, uint8_t *const dst);

forceinline void pdp11_cpu_instr_neg(Pdp11Cpu *const self, uint16_t *const dst);
forceinline void pdp11_cpu_instr_negb(Pdp11Cpu *const self, uint8_t *const dst);

forceinline void
pdp11_cpu_instr_tst(Pdp11Cpu *const self, uint16_t const *const src);
forceinline void
pdp11_cpu_instr_tstb(Pdp11Cpu *const self, uint8_t const *const src);

// NOTE this is a `COMplement` instruction, like `not` in intel
forceinline void pdp11_cpu_instr_com(Pdp11Cpu *const self, uint16_t *const dst);
forceinline void pdp11_cpu_instr_comb(Pdp11Cpu *const self, uint8_t *const dst);

// shifts

forceinline void pdp11_cpu_instr_asr(Pdp11Cpu *const self, uint16_t *const dst);
forceinline void pdp11_cpu_instr_asrb(Pdp11Cpu *const self, uint8_t *const dst);
forceinline void pdp11_cpu_instr_asl(Pdp11Cpu *const self, uint16_t *const dst);
forceinline void pdp11_cpu_instr_aslb(Pdp11Cpu *const self, uint8_t *const dst);

forceinline void pdp11_cpu_instr_ash(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
);
forceinline void pdp11_cpu_instr_ashc(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
);

// multiple-percision

forceinline void pdp11_cpu_instr_adc(Pdp11Cpu *const self, uint16_t *const dst);
forceinline void pdp11_cpu_instr_adcb(Pdp11Cpu *const self, uint8_t *const dst);
forceinline void pdp11_cpu_instr_sbc(Pdp11Cpu *const self, uint16_t *const dst);
forceinline void pdp11_cpu_instr_sbcb(Pdp11Cpu *const self, uint8_t *const dst);

forceinline void pdp11_cpu_instr_sxt(Pdp11Cpu *const self, uint16_t *const dst);

// rotates

forceinline void pdp11_cpu_instr_ror(Pdp11Cpu *const self, uint16_t *const dst);
forceinline void pdp11_cpu_instr_rorb(Pdp11Cpu *const self, uint8_t *const dst);
forceinline void pdp11_cpu_instr_rol(Pdp11Cpu *const self, uint16_t *const dst);
forceinline void pdp11_cpu_instr_rolb(Pdp11Cpu *const self, uint8_t *const dst);

forceinline void
pdp11_cpu_instr_swab(Pdp11Cpu *const self, uint16_t *const dst);

// DUAL-OP

// arithmetic

forceinline void pdp11_cpu_instr_mov(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
);
forceinline void pdp11_cpu_instr_movb(
    Pdp11Cpu *const self,
    uint8_t const *const src,
    uint8_t *const dst
);
forceinline void pdp11_cpu_instr_add(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
);
forceinline void pdp11_cpu_instr_sub(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
);
forceinline void pdp11_cpu_instr_cmp(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t const *const dst
);
forceinline void pdp11_cpu_instr_cmpb(
    Pdp11Cpu *const self,
    uint8_t const *const src,
    uint8_t const *const dst
);

// register destination

forceinline void pdp11_cpu_instr_mul(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
);
forceinline void pdp11_cpu_instr_div(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
);

forceinline void pdp11_cpu_instr_xor(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t *const dst
);

// logical

forceinline void pdp11_cpu_instr_bit(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t const *const dst
);
forceinline void pdp11_cpu_instr_bitb(
    Pdp11Cpu *const self,
    uint8_t const *const src,
    uint8_t const *const dst
);
forceinline void pdp11_cpu_instr_bis(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
);
forceinline void pdp11_cpu_instr_bisb(
    Pdp11Cpu *const self,
    uint8_t const *const src,
    uint8_t *const dst
);
forceinline void pdp11_cpu_instr_bic(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
);
forceinline void pdp11_cpu_instr_bicb(
    Pdp11Cpu *const self,
    uint8_t const *const src,
    uint8_t *const dst
);

// PROGRAM CONTROL

// branches

forceinline void pdp11_cpu_instr_br(Pdp11Cpu *const self, uint8_t const off);

forceinline void pdp11_cpu_instr_bne_be(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
);
forceinline void pdp11_cpu_instr_bpl_bmi(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
);
forceinline void pdp11_cpu_instr_bcc_bcs(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
);
forceinline void pdp11_cpu_instr_bvc_bvs(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
);

forceinline void pdp11_cpu_instr_bge_bl(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
);
forceinline void pdp11_cpu_instr_bg_ble(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
);

forceinline void pdp11_cpu_instr_bhi_blos(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
);
// NOTE `bhis`/`blo` is already implemented with `bcc`/`bcs`

// subroutine

forceinline void pdp11_cpu_instr_jsr(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
);
forceinline void
pdp11_cpu_instr_mark(Pdp11Cpu *const self, unsigned const param_count);
forceinline void pdp11_cpu_instr_rts(Pdp11Cpu *const self, unsigned const r_i);

// program control

forceinline void
pdp11_cpu_instr_spl(Pdp11Cpu *const self, unsigned const value);

forceinline void
pdp11_cpu_instr_jmp(Pdp11Cpu *const self, uint16_t const *const src);
forceinline void pdp11_cpu_instr_sob(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint8_t const off
);

// traps

forceinline void pdp11_cpu_instr_emt(Pdp11Cpu *const self);
forceinline void pdp11_cpu_instr_trap(Pdp11Cpu *const self);
forceinline void pdp11_cpu_instr_bpt(Pdp11Cpu *const self);
forceinline void pdp11_cpu_instr_iot(Pdp11Cpu *const self);
forceinline void pdp11_cpu_instr_rti(Pdp11Cpu *const self);
forceinline void pdp11_cpu_instr_rtt(Pdp11Cpu *const self);

// MISC.

forceinline void pdp11_cpu_instr_halt(Pdp11Cpu *const self);
forceinline void pdp11_cpu_instr_wait(Pdp11Cpu *const self);
forceinline void pdp11_cpu_instr_reset(Pdp11Cpu *const self);
// NOTE `nop` is already implemented with `clnzvc`/`senzvc`

forceinline void
pdp11_cpu_instr_mtpd(Pdp11Cpu *const self, uint16_t *const dst);
forceinline void
pdp11_cpu_instr_mtpi(Pdp11Cpu *const self, uint16_t *const dst);
forceinline void
pdp11_cpu_instr_mfpd(Pdp11Cpu *const self, uint16_t const *const src);
forceinline void
pdp11_cpu_instr_mfpi(Pdp11Cpu *const self, uint16_t const *const src);

// CONDITION CODES

forceinline void pdp11_cpu_instr_clnzvc_senzvc(
    Pdp11Cpu *const self,
    bool const value,
    bool const do_affect_nf,
    bool const do_affect_zf,
    bool const do_affect_vf,
    bool const do_affect_cf
);

/*************
 ** private **
 *************/

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

static inline void pdp11_cpu_stat_set_flags_from_word(
    Pdp11CpuStat *const self,
    uint16_t const value
) {
    *self = (Pdp11CpuStat){
        .priority = self->priority,
        .tf = self->tf,
        .nf = BIT(value, 15),
        .zf = value == 0,
        .vf = 0,
        .cf = self->cf,
    };
}
static inline void pdp11_cpu_stat_set_flags_from_byte(
    Pdp11CpuStat *const self,
    uint8_t const value
) {
    *self = (Pdp11CpuStat){
        .priority = self->priority,
        .tf = self->tf,
        .nf = BIT(value, 7),
        .zf = value == 0,
        .vf = 0,
        .cf = self->cf,
    };
}

static inline void pdp11_cpu_stat_set_flags_from_xword(
    Pdp11CpuStat *const self,
    uint32_t const value,
    bool const do_invert_cf
) {
    *self = (Pdp11CpuStat){
        .priority = self->priority,
        .tf = self->tf,
        .nf = BIT(value, 15),
        .zf = (uint16_t)value == 0,
        .vf = BIT(value, 16) != BIT(value, 15),
        .cf = BIT(value, 17) != do_invert_cf,
    };
}
static inline void pdp11_cpu_stat_set_flags_from_xbyte(
    Pdp11CpuStat *const self,
    uint16_t const value,
    bool const do_invert_cf
) {
    *self = (Pdp11CpuStat){
        .priority = self->priority,
        .tf = self->tf,
        .nf = BIT(value, 7),
        .zf = (uint8_t)value == 0,
        .vf = BIT(value, 8) != BIT(value, 7),
        .cf = BIT(value, 9) != do_invert_cf,
    };
}

static void pdp11_stack_push(Pdp11Cpu *const self, uint16_t const value) {
    pdp11_ram_word(self->_ram, pdp11_cpu_sp(self)) = value;
    pdp11_cpu_sp(self) -= 2;
}
static uint16_t pdp11_stack_pop(Pdp11Cpu *const self) {
    pdp11_cpu_sp(self) += 2;
    return pdp11_ram_word(self->_ram, pdp11_cpu_sp(self));
}

static inline void pdp11_cpu_trap(
    Pdp11Cpu *const self,
    Pdp11CpuTrap const trap /*,
    unsigned const priority */
) {
    pdp11_stack_push(self, pdp11_cpu_stat_to_word(&self->stat));
    pdp11_stack_push(self, pdp11_cpu_pc(self));
    pdp11_cpu_pc(self) = pdp11_ram_word(self->_ram, trap);
    self->stat = pdp11_cpu_stat(pdp11_ram_word(self->_ram, trap + 2));
}

static uint16_t *pdp11_address_word(Pdp11Cpu *const self, unsigned const mode) {
    // TODO trap CPU_ERR if odd address
    unsigned const r_i = BITS(mode, 0, 2);

    switch (BITS(mode, 3, 5)) {
    case 00: return &pdp11_cpu_rx(self, r_i);
    case 01: return &pdp11_ram_word(self->_ram, pdp11_cpu_rx(self, r_i));
    case 02: {
        uint16_t *const result =
            &pdp11_ram_word(self->_ram, pdp11_cpu_rx(self, r_i));
        pdp11_cpu_rx(self, r_i) += 2;
        return result;
    } break;
    case 03: {
        uint16_t *const result = &pdp11_ram_word(
            self->_ram,
            pdp11_ram_word(self->_ram, pdp11_cpu_rx(self, r_i))
        );
        pdp11_cpu_rx(self, r_i) += 2;
        return result;
    } break;
    case 04: return &pdp11_ram_word(self->_ram, pdp11_cpu_rx(self, r_i) -= 2);
    case 05:
        return &pdp11_ram_word(
            self->_ram,
            pdp11_ram_word(self->_ram, pdp11_cpu_rx(self, r_i) -= 2)
        );
    case 06: {
        uint16_t const reg_val = pdp11_cpu_rx(self, r_i);
        return &pdp11_ram_word(
            self->_ram,
            reg_val + pdp11_ram_word(self->_ram, pdp11_cpu_pc(self)++)
        );
    } break;
    case 07: {
        uint16_t const reg_val = pdp11_cpu_rx(self, r_i);
        return &pdp11_ram_word(
            self->_ram,
            pdp11_ram_word(
                self->_ram,
                reg_val + pdp11_ram_word(self->_ram, pdp11_cpu_pc(self)++)
            )
        );
    } break;
    }

    return NULL;
}
static uint8_t *pdp11_address_byte(Pdp11Cpu *const self, unsigned const mode) {
    unsigned const r_i = BITS(mode, 0, 2);

    switch (BITS(mode, 3, 5)) {
    case 00: return &pdp11_cpu_rl(self, r_i);
    case 01: return &pdp11_ram_byte(self->_ram, pdp11_cpu_rx(self, r_i));
    case 02: {
        uint8_t *const result =
            &pdp11_ram_byte(self->_ram, pdp11_cpu_rx(self, r_i));
        pdp11_cpu_rx(self, r_i) += r_i >= 06 ? 2 : 1;
        return result;
    } break;
    case 03: {
        uint8_t *const result = &pdp11_ram_byte(
            self->_ram,
            pdp11_ram_word(self->_ram, pdp11_cpu_rx(self, r_i))
        );
        pdp11_cpu_rx(self, r_i) += 2;
        return result;
    } break;
    case 04:
        return &pdp11_ram_byte(
            self->_ram,
            pdp11_cpu_rx(self, r_i) -= r_i >= 06 ? 2 : 1
        );
    case 05:
        return &pdp11_ram_byte(
            self->_ram,
            pdp11_ram_word(self->_ram, pdp11_cpu_rx(self, r_i) -= 2)
        );
    case 06: {
        uint16_t const reg_val = pdp11_cpu_rx(self, r_i);
        return &pdp11_ram_byte(
            self->_ram,
            reg_val + pdp11_ram_word(self->_ram, pdp11_cpu_pc(self)++)
        );
    } break;
    case 07: {
        uint16_t const reg_val = pdp11_cpu_rx(self, r_i);
        return &pdp11_ram_byte(
            self->_ram,
            pdp11_ram_word(
                self->_ram,
                reg_val + pdp11_ram_word(self->_ram, pdp11_cpu_pc(self)++)
            )
        );
    } break;
    }

    return NULL;
}
static void
pdp11_cpu_exec_instr_helper(Pdp11Cpu *const self, uint16_t const instr) {
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
        return pdp11_cpu_instr_mov(
            self,
            pdp11_address_word(self, op_11_6),
            pdp11_address_word(self, op_5_0)
        );
    case 011:
        return pdp11_cpu_instr_movb(
            self,
            pdp11_address_byte(self, op_11_6),
            pdp11_address_byte(self, op_5_0)
        );
    case 002:
        return pdp11_cpu_instr_cmp(
            self,
            pdp11_address_word(self, op_11_6),
            pdp11_address_word(self, op_5_0)
        );
    case 012:
        return pdp11_cpu_instr_cmpb(
            self,
            pdp11_address_byte(self, op_11_6),
            pdp11_address_byte(self, op_5_0)
        );
    case 003:
        return pdp11_cpu_instr_bit(
            self,
            pdp11_address_word(self, op_11_6),
            pdp11_address_word(self, op_5_0)
        );
    case 013:
        return pdp11_cpu_instr_bitb(
            self,
            pdp11_address_byte(self, op_11_6),
            pdp11_address_byte(self, op_5_0)
        );
    case 004:
        return pdp11_cpu_instr_bic(
            self,
            pdp11_address_word(self, op_11_6),
            pdp11_address_word(self, op_5_0)
        );
    case 014:
        return pdp11_cpu_instr_bicb(
            self,
            pdp11_address_byte(self, op_11_6),
            pdp11_address_byte(self, op_5_0)
        );
    case 005:
        return pdp11_cpu_instr_bis(
            self,
            pdp11_address_word(self, op_11_6),
            pdp11_address_word(self, op_5_0)
        );
    case 015:
        return pdp11_cpu_instr_bisb(
            self,
            pdp11_address_byte(self, op_11_6),
            pdp11_address_byte(self, op_5_0)
        );
    case 006:
        return pdp11_cpu_instr_add(
            self,
            pdp11_address_word(self, op_11_6),
            pdp11_address_word(self, op_5_0)
        );
    case 016:
        return pdp11_cpu_instr_sub(
            self,
            pdp11_address_word(self, op_11_6),
            pdp11_address_word(self, op_5_0)
        );
    }
    switch (opcode_15_9) {
    case 0070:
        return pdp11_cpu_instr_mul(
            self,
            op_8_6,
            pdp11_address_word(self, op_5_0)
        );
    case 0071:
        return pdp11_cpu_instr_div(
            self,
            op_8_6,
            pdp11_address_word(self, op_5_0)
        );
    case 0072:
        return pdp11_cpu_instr_ash(
            self,
            op_8_6,
            pdp11_address_word(self, op_5_0)
        );
    case 0073:
        return pdp11_cpu_instr_ashc(
            self,
            op_8_6,
            pdp11_address_word(self, op_5_0)
        );
    case 0074:
        return pdp11_cpu_instr_xor(
            self,
            op_8_6,
            pdp11_address_word(self, op_5_0)
        );

    case 0000:
        if (op_8 != 1) break;
        return pdp11_cpu_instr_br(self, op_7_0);
    case 0001: return pdp11_cpu_instr_bne_be(self, op_8, op_7_0);
    case 0002: return pdp11_cpu_instr_bge_bl(self, op_8, op_7_0);
    case 0003: return pdp11_cpu_instr_bg_ble(self, op_8, op_7_0);
    case 0100: return pdp11_cpu_instr_bpl_bmi(self, op_8, op_7_0);
    case 0101: return pdp11_cpu_instr_bhi_blos(self, op_8, op_7_0);
    case 0102: return pdp11_cpu_instr_bvc_bvs(self, op_8, op_7_0);
    case 0103: return pdp11_cpu_instr_bcc_bcs(self, op_8, op_7_0);

    case 0077: return pdp11_cpu_instr_sob(self, op_8_6, op_5_0);

    case 0004:
        return pdp11_cpu_instr_jsr(
            self,
            op_8_6,
            pdp11_address_word(self, op_5_0)
        );

    case 0104:
        return op_8 ? pdp11_cpu_instr_emt(self) : pdp11_cpu_instr_trap(self);
    }
    switch (opcode_15_6) {
    case 00001:
        return pdp11_cpu_instr_jmp(self, pdp11_address_word(self, op_5_0));
    case 00003:
        return pdp11_cpu_instr_swab(self, pdp11_address_word(self, op_5_0));
    case 00050:
        return pdp11_cpu_instr_clr(self, pdp11_address_word(self, op_5_0));
    case 01050:
        return pdp11_cpu_instr_clrb(self, pdp11_address_byte(self, op_5_0));
    case 00051:
        return pdp11_cpu_instr_com(self, pdp11_address_word(self, op_5_0));
    case 01051:
        return pdp11_cpu_instr_comb(self, pdp11_address_byte(self, op_5_0));
    case 00052:
        return pdp11_cpu_instr_inc(self, pdp11_address_word(self, op_5_0));
    case 01052:
        return pdp11_cpu_instr_incb(self, pdp11_address_byte(self, op_5_0));
    case 00053:
        return pdp11_cpu_instr_dec(self, pdp11_address_word(self, op_5_0));
    case 01053:
        return pdp11_cpu_instr_decb(self, pdp11_address_byte(self, op_5_0));
    case 00054:
        return pdp11_cpu_instr_neg(self, pdp11_address_word(self, op_5_0));
    case 01054:
        return pdp11_cpu_instr_negb(self, pdp11_address_byte(self, op_5_0));
    case 00055:
        return pdp11_cpu_instr_adc(self, pdp11_address_word(self, op_5_0));
    case 01055:
        return pdp11_cpu_instr_adcb(self, pdp11_address_byte(self, op_5_0));
    case 00056:
        return pdp11_cpu_instr_sbc(self, pdp11_address_word(self, op_5_0));
    case 01056:
        return pdp11_cpu_instr_sbcb(self, pdp11_address_byte(self, op_5_0));
    case 00057:
        return pdp11_cpu_instr_tst(self, pdp11_address_word(self, op_5_0));
    case 01057:
        return pdp11_cpu_instr_tstb(self, pdp11_address_byte(self, op_5_0));
    case 00060:
        return pdp11_cpu_instr_ror(self, pdp11_address_word(self, op_5_0));
    case 01060:
        return pdp11_cpu_instr_rorb(self, pdp11_address_byte(self, op_5_0));
    case 00061:
        return pdp11_cpu_instr_rol(self, pdp11_address_word(self, op_5_0));
    case 01061:
        return pdp11_cpu_instr_rolb(self, pdp11_address_byte(self, op_5_0));
    case 00062:
        return pdp11_cpu_instr_asr(self, pdp11_address_word(self, op_5_0));
    case 01062:
        return pdp11_cpu_instr_asrb(self, pdp11_address_byte(self, op_5_0));
    case 00063:
        return pdp11_cpu_instr_asl(self, pdp11_address_word(self, op_5_0));
    case 01063:
        return pdp11_cpu_instr_aslb(self, pdp11_address_byte(self, op_5_0));
    case 00065:
        return pdp11_cpu_instr_mfpi(self, pdp11_address_word(self, op_5_0));
    case 01065:
        return pdp11_cpu_instr_mfpd(self, pdp11_address_word(self, op_5_0));
    case 00066:
        return pdp11_cpu_instr_mtpi(self, pdp11_address_word(self, op_5_0));
    case 01066:
        return pdp11_cpu_instr_mtpd(self, pdp11_address_word(self, op_5_0));
    case 00067:
        return pdp11_cpu_instr_sxt(self, pdp11_address_word(self, op_5_0));

    case 00064: return pdp11_cpu_instr_mark(self, op_5_0);

    case 00002:
        if (!BIT(instr, 5)) break;
        return pdp11_cpu_instr_clnzvc_senzvc(
            self,
            BIT(instr, 4),
            BIT(instr, 3),
            BIT(instr, 2),
            BIT(instr, 1),
            BIT(instr, 0)
        );
    }
    switch (opcode_15_3) {
    case 000020: return pdp11_cpu_instr_rts(self, op_2_0);
    case 000023: return pdp11_cpu_instr_spl(self, op_2_0);
    }
    switch (opcode_15_0) {
    case 0000002: return pdp11_cpu_instr_rti(self);
    case 0000003: return pdp11_cpu_instr_bpt(self);
    case 0000004: return pdp11_cpu_instr_iot(self);
    case 0000006: return pdp11_cpu_instr_rtt(self);

    case 0000000: return pdp11_cpu_instr_halt(self);
    case 0000001: return pdp11_cpu_instr_wait(self);
    case 0000005: return pdp11_cpu_instr_reset(self);
    }

    pdp11_cpu_trap(self, PDP11_CPU_TRAP_ILLEGAL_INSTR);
}

/************
 ** public **
 ************/

void pdp11_cpu_init(
    Pdp11Cpu *const self,
    Pdp11Ram *const ram,
    uint16_t const pc,
    Pdp11CpuStat const stat
) {
    pdp11_cpu_pc(self) = pc;
    self->stat = stat;

    self->_ram = ram;
}
void pdp11_cpu_uninit(Pdp11Cpu *const self) {
    pdp11_cpu_pc(self) = 0;
    self->stat = (Pdp11CpuStat){0};
}

void pdp11_cpu_exec_instr(Pdp11Cpu *const self, uint16_t const instr) {
    pdp11_cpu_exec_instr_helper(self, instr);
    if (self->stat.tf) pdp11_cpu_trap(self, PDP11_CPU_TRAP_BPT);
}

/****************
 ** instr impl **
 ****************/

// SINGLE-OP

// general

void pdp11_cpu_instr_clr(Pdp11Cpu *const self, uint16_t *const dst) {
    *dst = 0;
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = 0,
        .zf = 1,
        .vf = 0,
        .cf = 0,
    };
}
void pdp11_cpu_instr_clrb(Pdp11Cpu *const self, uint8_t *const dst) {
    *dst = 0;
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = 0,
        .zf = 1,
        .vf = 0,
        .cf = 0,
    };
}
void pdp11_cpu_instr_inc(Pdp11Cpu *const self, uint16_t *const dst) {
    (*dst)++;
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*dst, 15),
        .zf = *dst == 0,
        .vf = *dst == 0x8000,
        .cf = self->stat.cf,
    };
}
void pdp11_cpu_instr_incb(Pdp11Cpu *const self, uint8_t *const dst) {
    (*dst)++;
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*dst, 7),
        .zf = *dst == 0,
        .vf = *dst == 0x80,
        .cf = self->stat.cf,
    };
}
void pdp11_cpu_instr_dec(Pdp11Cpu *const self, uint16_t *const dst) {
    (*dst)--;
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*dst, 15),
        .zf = *dst == 0,
        .vf = *dst == 0x7FFF,
        .cf = self->stat.cf,
    };
}
void pdp11_cpu_instr_decb(Pdp11Cpu *const self, uint8_t *const dst) {
    (*dst)--;
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*dst, 7),
        .zf = *dst == 0,
        .vf = *dst == 0x7F,
        .cf = self->stat.cf,
    };
}

void pdp11_cpu_instr_neg(Pdp11Cpu *const self, uint16_t *const dst) {
    *dst = -*dst;
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*dst, 15),
        .zf = *dst == 0,
        .vf = *dst == 0x8000,
        .cf = *dst != 0,
    };
}
void pdp11_cpu_instr_negb(Pdp11Cpu *const self, uint8_t *const dst) {
    *dst = -*dst;
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*dst, 7),
        .zf = *dst == 0,
        .vf = *dst == 0x80,
        .cf = *dst != 0,
    };
}

void pdp11_cpu_instr_tst(Pdp11Cpu *const self, uint16_t const *const src) {
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*src, 15),
        .zf = *src == 0,
        .vf = 0,
        .cf = 0,
    };
}
void pdp11_cpu_instr_tstb(Pdp11Cpu *const self, uint8_t const *const src) {
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*src, 7),
        .zf = *src == 0,
        .vf = 0,
        .cf = 0,
    };
}

void pdp11_cpu_instr_com(Pdp11Cpu *const self, uint16_t *const dst) {
    *dst = ~*dst;
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*dst, 15),
        .zf = *dst == 0,
        .vf = 0,
        .cf = 1,
    };
}
void pdp11_cpu_instr_comb(Pdp11Cpu *const self, uint8_t *const dst) {
    *dst = ~*dst;
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*dst, 7),
        .zf = *dst == 0,
        .vf = 0,
        .cf = 1,
    };
}

// shifts

void pdp11_cpu_instr_asr(Pdp11Cpu *const self, uint16_t *const dst) {
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*dst, 15),
        .cf = BIT(*dst, 0),
    };

    *dst >>= 1;

    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = self->stat.nf,
        .zf = *dst == 0,
        .vf = self->stat.nf ^ self->stat.cf,
        .cf = self->stat.cf,
    };
}
void pdp11_cpu_instr_asrb(Pdp11Cpu *const self, uint8_t *const dst) {
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*dst, 7),
        .cf = BIT(*dst, 0),
    };

    *dst >>= 1;

    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = self->stat.nf,
        .zf = *dst == 0,
        .vf = self->stat.nf ^ self->stat.cf,
        .cf = self->stat.cf,
    };
}
void pdp11_cpu_instr_asl(Pdp11Cpu *const self, uint16_t *const dst) {
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*dst, 14),
        .cf = BIT(*dst, 15),
    };

    *dst <<= 1;

    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = self->stat.nf,
        .zf = *dst == 0,
        .vf = self->stat.nf ^ self->stat.cf,
        .cf = self->stat.cf,
    };
}
void pdp11_cpu_instr_aslb(Pdp11Cpu *const self, uint8_t *const dst) {
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*dst, 6),
        .cf = BIT(*dst, 7),
    };

    *dst <<= 1;

    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = self->stat.nf,
        .zf = *dst == 0,
        .vf = self->stat.nf ^ self->stat.cf,
        .cf = self->stat.cf,
    };
}

void pdp11_cpu_instr_ash(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
) {
    /* uint16_t *const rx = &pdp11_cpu_rx(self, r_i);


    self->stat = (Pdp11CpuStat){
        .priority =self->stat.priority,
        .tf =self->stat.tf,
        .nf = BIT(*rx, 15),
        .cf = BIT(*rx, 0),
    };

    int8_t const shift_amount = *src & 0x1F;
    *rx = shift_amount >= 0 ? (*rx << shift_amount) : (*rx >> -shift_amount);

    self->stat = (Pdp11CpuStat){
        .priority =self->stat.priority,
        .tf =self->stat.tf,
        .nf = BIT(*rx, 15),
        .zf = *rx == 0,
        .vf = BIT(*rx, 15) !=self->stat.nf,
    }; */
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_cpu_instr_ashc(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}

// multiple-percision

void pdp11_cpu_instr_adc(Pdp11Cpu *const self, uint16_t *const dst) {
    *dst += self->stat.cf;
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*dst, 15),
        .zf = *dst == 0,
        .vf = self->stat.cf && *dst == 0x8000,
        .cf = self->stat.cf && *dst == 0x0000,
    };
}
void pdp11_cpu_instr_adcb(Pdp11Cpu *const self, uint8_t *const dst) {
    *dst += self->stat.cf;
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*dst, 7),
        .zf = *dst == 0,
        .vf = self->stat.cf && *dst == 0x80,
        .cf = self->stat.cf && *dst == 0x00,
    };
}
void pdp11_cpu_instr_sbc(Pdp11Cpu *const self, uint16_t *const dst) {
    *dst -= self->stat.cf;
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*dst, 15),
        .zf = *dst == 0,
        .vf = self->stat.cf && *dst == 0x7FFF,
        .cf = self->stat.cf && *dst == 0xFFFF,
    };
}
void pdp11_cpu_instr_sbcb(Pdp11Cpu *const self, uint8_t *const dst) {
    *dst -= self->stat.cf;
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*dst, 7),
        .zf = *dst == 0,
        .vf = self->stat.cf && *dst == 0x7F,
        .cf = self->stat.cf && *dst == 0xFF,
    };
}

void pdp11_cpu_instr_sxt(Pdp11Cpu *const self, uint16_t *const dst) {
    *dst = self->stat.nf ? 0xFFFF : 0x0000;
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = self->stat.nf,
        .zf = !self->stat.nf,
        .vf = 0,
        .cf = self->stat.cf,
    };
}

// rotates

void pdp11_cpu_instr_ror(Pdp11Cpu *const self, uint16_t *const dst) {
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = self->stat.cf,
        .cf = BIT(*dst, 0),
    };

    *dst = (*dst >> 1) | (self->stat.cf << 15);

    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = self->stat.nf,
        .zf = *dst == 0,
        .vf = self->stat.nf ^ self->stat.cf,
        .cf = self->stat.cf,
    };
}
void pdp11_cpu_instr_rorb(Pdp11Cpu *const self, uint8_t *const dst) {
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = self->stat.cf,
        .cf = BIT(*dst, 0),
    };

    *dst = (*dst >> 1) | (self->stat.cf << 7);

    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = self->stat.nf,
        .zf = *dst == 0,
        .vf = self->stat.nf ^ self->stat.cf,
        .cf = self->stat.cf,
    };
}
void pdp11_cpu_instr_rol(Pdp11Cpu *const self, uint16_t *const dst) {
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = self->stat.cf,  // TODO refactor this crappy line
        .cf = BIT(*dst, 15),
    };

    *dst = (*dst << 1) | self->stat.nf;

    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*dst, 15),
        .zf = *dst == 0,
        .vf = self->stat.nf ^ self->stat.cf,
        .cf = self->stat.cf,
    };
}
void pdp11_cpu_instr_rolb(Pdp11Cpu *const self, uint8_t *const dst) {
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = self->stat.cf,  // TODO refactor this crappy line
        .cf = BIT(*dst, 7),
    };

    *dst = (*dst << 1) | self->stat.nf;

    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*dst, 7),
        .zf = *dst == 0,
        .vf = self->stat.nf ^ self->stat.cf,
        .cf = self->stat.cf,
    };
}

void pdp11_cpu_instr_swab(Pdp11Cpu *const self, uint16_t *const dst) {
    *dst = (uint16_t)(*dst << 8) | (uint8_t)(*dst >> 8);
    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(*dst, 7),
        .zf = (uint8_t)*dst == 0,
        .vf = 0,
        .cf = 0,
    };
}

// DUAL-OP

// arithmetic

void pdp11_cpu_instr_mov(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
) {
    *dst = *src;
    pdp11_cpu_stat_set_flags_from_word(&self->stat, *src);
}
void pdp11_cpu_instr_movb(
    Pdp11Cpu *const self,
    uint8_t const *const src,
    uint8_t *const dst
) {
    uint16_t *const dst16 = (uint16_t *)dst;
    if ((uintptr_t)dst16 % alignof(uint16_t) == 0 &&
        &pdp11_cpu_rx(self, 0) <= dst16 &&
        dst16 <= &pdp11_cpu_rx(self, PDP11_CPU_REG_COUNT - 1)) {
        *dst16 = (int16_t)(int8_t)*src;
    } else {
        *dst = *src;
    }
    pdp11_cpu_stat_set_flags_from_byte(&self->stat, *dst);
}
void pdp11_cpu_instr_add(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
) {
    uint32_t const result = xword(*src) + xword(*dst);
    *dst = result;
    pdp11_cpu_stat_set_flags_from_xword(&self->stat, result, false);
}
void pdp11_cpu_instr_sub(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
) {
    uint32_t const result = xword(*src) + xword(-*dst);
    *dst = result;
    pdp11_cpu_stat_set_flags_from_xword(&self->stat, result, true);
}
void pdp11_cpu_instr_cmp(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t const *const dst
) {
    pdp11_cpu_stat_set_flags_from_xword(
        &self->stat,
        xword(*src) + xword(-*dst),
        true
    );
}
void pdp11_cpu_instr_cmpb(
    Pdp11Cpu *const self,
    uint8_t const *const src,
    uint8_t const *const dst
) {
    pdp11_cpu_stat_set_flags_from_xbyte(
        &self->stat,
        xbyte(*src) + xbyte(-*dst),
        true
    );
}

// register destination

void pdp11_cpu_instr_mul(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
) {
    uint32_t const result =
        (uint32_t)((int16_t)*src * (int16_t)pdp11_cpu_rx(self, r_i));
    if ((r_i & 1) == 0) pdp11_cpu_rx(self, r_i | 1) = (uint16_t)(result >> 16);
    pdp11_cpu_rx(self, r_i) = (uint16_t)result;

    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(result, 15),
        .zf = (uint16_t)result == 0,
        .vf = 0,
        .cf = (uint16_t)(result >> 16) != (BIT(result, 15) ? 0xFFFF : 0x0000),
    };
}
void pdp11_cpu_instr_div(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
) {
    if (*src == 0) {
        self->stat.vf = self->stat.cf = 1;
        return;
    }

    uint32_t dst_value = pdp11_cpu_rx(self, r_i);
    if ((r_i & 1) == 0) dst_value |= pdp11_cpu_rx(self, r_i | 1) << 16;

    uint32_t const quotient = dst_value / *src;

    if ((uint16_t)(quotient >> 16) != 0) {
        self->stat.vf = 1;
        return;
    }

    pdp11_cpu_rx(self, r_i) = quotient;
    if ((r_i & 1) == 0) pdp11_cpu_rx(self, r_i) = dst_value % *src;

    self->stat = (Pdp11CpuStat){
        .priority = self->stat.priority,
        .tf = self->stat.tf,
        .nf = BIT(quotient, 15),
        .zf = quotient == 0,
        .vf = 0,
        .cf = 0,
    };
}

void pdp11_cpu_instr_xor(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t *const dst
) {
    *dst ^= pdp11_cpu_rx(self, r_i);
    pdp11_cpu_stat_set_flags_from_word(&self->stat, *dst);
}

// logical

void pdp11_cpu_instr_bit(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t const *const dst
) {
    pdp11_cpu_stat_set_flags_from_word(&self->stat, *dst & *src);
}
void pdp11_cpu_instr_bitb(
    Pdp11Cpu *const self,
    uint8_t const *const src,
    uint8_t const *const dst
) {
    pdp11_cpu_stat_set_flags_from_byte(&self->stat, *dst & *src);
}
void pdp11_cpu_instr_bis(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
) {
    *dst |= *src;
    pdp11_cpu_instr_bit(self, dst, dst);
}
void pdp11_cpu_instr_bisb(
    Pdp11Cpu *const self,
    uint8_t const *const src,
    uint8_t *const dst
) {
    *dst |= *src;
    pdp11_cpu_instr_bitb(self, dst, dst);
}
void pdp11_cpu_instr_bic(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
) {
    *dst &= ~*src;
    pdp11_cpu_instr_bit(self, dst, dst);
}
void pdp11_cpu_instr_bicb(
    Pdp11Cpu *const self,
    uint8_t const *const src,
    uint8_t *const dst
) {
    *dst &= ~*src;
    pdp11_cpu_instr_bitb(self, dst, dst);
}

// PROGRAM CONTROL

// branches

void pdp11_cpu_instr_br(Pdp11Cpu *const self, uint8_t const off) {
    pdp11_cpu_pc(self) += 2 * (int16_t)(int8_t)off;
}

void pdp11_cpu_instr_bne_be(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
) {
    if (self->stat.zf == cond) pdp11_cpu_instr_br(self, off);
}
void pdp11_cpu_instr_bpl_bmi(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
) {
    if (self->stat.nf == cond) pdp11_cpu_instr_br(self, off);
}
void pdp11_cpu_instr_bcc_bcs(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
) {
    if (self->stat.cf == cond) pdp11_cpu_instr_br(self, off);
}
void pdp11_cpu_instr_bvc_bvs(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
) {
    if (self->stat.vf == cond) pdp11_cpu_instr_br(self, off);
}

void pdp11_cpu_instr_bge_bl(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
) {
    if ((self->stat.nf ^ self->stat.vf) == cond) pdp11_cpu_instr_br(self, off);
}
void pdp11_cpu_instr_bg_ble(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
) {
    if ((self->stat.zf | (self->stat.nf ^ self->stat.vf)) == cond)
        pdp11_cpu_instr_br(self, off);
}

void pdp11_cpu_instr_bhi_blos(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
) {
    if ((self->stat.cf | self->stat.zf) == cond) pdp11_cpu_instr_br(self, off);
}

// subroutine

void pdp11_cpu_instr_jsr(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
) {
    pdp11_stack_push(self, pdp11_cpu_rx(self, r_i));
    pdp11_cpu_rx(self, r_i) = pdp11_cpu_pc(self);
    pdp11_cpu_pc(self) = *src;
}
void pdp11_cpu_instr_mark(Pdp11Cpu *const self, unsigned const param_count) {
    pdp11_cpu_sp(self) = pdp11_cpu_pc(self) + 2 * param_count;
    pdp11_cpu_pc(self) = pdp11_cpu_rx(self, 5);
    pdp11_cpu_pc(self) = pdp11_stack_pop(self);
}
void pdp11_cpu_instr_rts(Pdp11Cpu *const self, unsigned const r_i) {
    pdp11_cpu_pc(self) = pdp11_cpu_rx(self, r_i);
    pdp11_cpu_rx(self, r_i) = pdp11_stack_pop(self);
}

// program control

void pdp11_cpu_instr_spl(Pdp11Cpu *const self, unsigned const value) {
    self->stat.priority = value;
}

void pdp11_cpu_instr_jmp(Pdp11Cpu *const self, uint16_t const *const src) {
    pdp11_cpu_pc(self) = *src;
}
void pdp11_cpu_instr_sob(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint8_t const off
) {
    if (--pdp11_cpu_rx(self, r_i) != 0) pdp11_cpu_pc(self) -= 2 * off;
}

// trap

void pdp11_cpu_instr_emt(Pdp11Cpu *const self) {
    pdp11_cpu_trap(self, PDP11_CPU_TRAP_EMT);
}
void pdp11_cpu_instr_trap(Pdp11Cpu *const self) {
    // TODO it user data could be passed through the low byte
    pdp11_cpu_trap(self, PDP11_CPU_TRAP_TRAP);
}
void pdp11_cpu_instr_bpt(Pdp11Cpu *const self) {
    pdp11_cpu_trap(self, PDP11_CPU_TRAP_BPT);
}
void pdp11_cpu_instr_iot(Pdp11Cpu *const self) {
    pdp11_cpu_trap(self, PDP11_CPU_TRAP_IOT);
}
// TODO rti and rtt should be slightply different, but could not understand why
// at this point
void pdp11_cpu_instr_rti(Pdp11Cpu *const self) {
    pdp11_cpu_pc(self) = pdp11_stack_pop(self);
    self->stat = pdp11_cpu_stat(pdp11_stack_pop(self));
}
void pdp11_cpu_instr_rtt(Pdp11Cpu *const self) {
    pdp11_cpu_pc(self) = pdp11_stack_pop(self);
    self->stat = pdp11_cpu_stat(pdp11_stack_pop(self));
}

// MISC.

void pdp11_cpu_instr_halt(Pdp11Cpu *const self) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_cpu_instr_wait(Pdp11Cpu *const self) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_cpu_instr_reset(Pdp11Cpu *const self) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}

void pdp11_cpu_instr_mtpd(Pdp11Cpu *const self, uint16_t *const dst) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_cpu_instr_mtpi(Pdp11Cpu *const self, uint16_t *const dst) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_cpu_instr_mfpd(Pdp11Cpu *const self, uint16_t const *const src) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_cpu_instr_mfpi(Pdp11Cpu *const self, uint16_t const *const src) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}

// CONDITION CODES

void pdp11_cpu_instr_clnzvc_senzvc(
    Pdp11Cpu *const self,
    bool const value,
    bool const do_affect_nf,
    bool const do_affect_zf,
    bool const do_affect_vf,
    bool const do_affect_cf
) {
    if (do_affect_nf) self->stat.nf = value;
    if (do_affect_zf) self->stat.zf = value;
    if (do_affect_vf) self->stat.vf = value;
    if (do_affect_cf) self->stat.cf = value;
}
