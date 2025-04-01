#include "pdp11/cpu/pdp11_cpu_instr_set.h"

#include <stdio.h>

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

static inline void pdp11_request_intr(
    Pdp11Cpu *const self,
    uint16_t const addr /*,
    unsigned const priority */
) {
    pdp11_stack_push(self, pdp11_cpu_stat_to_word(&self->_stat));
    pdp11_stack_push(self, pdp11_cpu_pc(self));
    pdp11_cpu_pc(self) = *pdp11_ram_word(self->_ram, addr);
    self->_stat = pdp11_cpu_stat(*pdp11_ram_word(self->_ram, addr + 2));
}

/************
 ** public **
 ************/

// SINGLE-OP

// general

void pdp11_op_clr(Pdp11Cpu *const self, uint16_t *const dst) {
    *dst = 0;
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = 0,
        .zf = 1,
        .vf = 0,
        .cf = 0,
    };
}
void pdp11_op_clrb(Pdp11Cpu *const self, uint8_t *const dst) {
    *dst = 0;
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = 0,
        .zf = 1,
        .vf = 0,
        .cf = 0,
    };
}
void pdp11_op_inc(Pdp11Cpu *const self, uint16_t *const dst) {
    (*dst)++;
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*dst, 15),
        .zf = *dst == 0,
        .vf = *dst == 0x8000,
        .cf = self->_stat.cf,
    };
}
void pdp11_op_incb(Pdp11Cpu *const self, uint8_t *const dst) {
    (*dst)++;
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*dst, 7),
        .zf = *dst == 0,
        .vf = *dst == 0x80,
        .cf = self->_stat.cf,
    };
}
void pdp11_op_dec(Pdp11Cpu *const self, uint16_t *const dst) {
    (*dst)--;
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*dst, 15),
        .zf = *dst == 0,
        .vf = *dst == 0x7FFF,
        .cf = self->_stat.cf,
    };
}
void pdp11_op_decb(Pdp11Cpu *const self, uint8_t *const dst) {
    (*dst)--;
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*dst, 7),
        .zf = *dst == 0,
        .vf = *dst == 0x7F,
        .cf = self->_stat.cf,
    };
}

void pdp11_op_neg(Pdp11Cpu *const self, uint16_t *const dst) {
    *dst = -*dst;
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*dst, 15),
        .zf = *dst == 0,
        .vf = *dst == 0x8000,
        .cf = *dst != 0,
    };
}
void pdp11_op_negb(Pdp11Cpu *const self, uint8_t *const dst) {
    *dst = -*dst;
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*dst, 7),
        .zf = *dst == 0,
        .vf = *dst == 0x80,
        .cf = *dst != 0,
    };
}

void pdp11_op_tst(Pdp11Cpu *const self, uint16_t const *const src) {
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*src, 15),
        .zf = *src == 0,
        .vf = 0,
        .cf = 0,
    };
}
void pdp11_op_tstb(Pdp11Cpu *const self, uint8_t const *const src) {
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*src, 7),
        .zf = *src == 0,
        .vf = 0,
        .cf = 0,
    };
}

void pdp11_op_com(Pdp11Cpu *const self, uint16_t *const dst) {
    *dst = ~*dst;
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*dst, 15),
        .zf = *dst == 0,
        .vf = 0,
        .cf = 1,
    };
}
void pdp11_op_comb(Pdp11Cpu *const self, uint8_t *const dst) {
    *dst = ~*dst;
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*dst, 7),
        .zf = *dst == 0,
        .vf = 0,
        .cf = 1,
    };
}

// shifts

void pdp11_op_asr(Pdp11Cpu *const self, uint16_t *const dst) {
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*dst, 15),
        .cf = BIT(*dst, 0),
    };

    *dst >>= 1;

    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = self->_stat.nf,
        .zf = *dst == 0,
        .vf = self->_stat.nf ^ self->_stat.cf,
        .cf = self->_stat.cf,
    };
}
void pdp11_op_asrb(Pdp11Cpu *const self, uint8_t *const dst) {
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*dst, 7),
        .cf = BIT(*dst, 0),
    };

    *dst >>= 1;

    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = self->_stat.nf,
        .zf = *dst == 0,
        .vf = self->_stat.nf ^ self->_stat.cf,
        .cf = self->_stat.cf,
    };
}
void pdp11_op_asl(Pdp11Cpu *const self, uint16_t *const dst) {
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*dst, 14),
        .cf = BIT(*dst, 15),
    };

    *dst <<= 1;

    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = self->_stat.nf,
        .zf = *dst == 0,
        .vf = self->_stat.nf ^ self->_stat.cf,
        .cf = self->_stat.cf,
    };
}
void pdp11_op_aslb(Pdp11Cpu *const self, uint8_t *const dst) {
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*dst, 6),
        .cf = BIT(*dst, 7),
    };

    *dst <<= 1;

    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = self->_stat.nf,
        .zf = *dst == 0,
        .vf = self->_stat.nf ^ self->_stat.cf,
        .cf = self->_stat.cf,
    };
}

void pdp11_op_ash(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
) {
    /* uint16_t *const rx = &pdp11_cpu_rx(self, r_i);


    self->_stat = (Pdp11CpuStat){
        .priority =self->_stat.priority,
        .tf =self->_stat.tf,
        .nf = BIT(*rx, 15),
        .cf = BIT(*rx, 0),
    };

    int8_t const shift_amount = *src & 0x1F;
    *rx = shift_amount >= 0 ? (*rx << shift_amount) : (*rx >> -shift_amount);

    self->_stat = (Pdp11CpuStat){
        .priority =self->_stat.priority,
        .tf =self->_stat.tf,
        .nf = BIT(*rx, 15),
        .zf = *rx == 0,
        .vf = BIT(*rx, 15) !=self->_stat.nf,
    }; */
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_ashc(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}

// multiple-percision

void pdp11_op_adc(Pdp11Cpu *const self, uint16_t *const dst) {
    *dst += self->_stat.cf;
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*dst, 15),
        .zf = *dst == 0,
        .vf = self->_stat.cf && *dst == 0x8000,
        .cf = self->_stat.cf && *dst == 0x0000,
    };
}
void pdp11_op_adcb(Pdp11Cpu *const self, uint8_t *const dst) {
    *dst += self->_stat.cf;
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*dst, 7),
        .zf = *dst == 0,
        .vf = self->_stat.cf && *dst == 0x80,
        .cf = self->_stat.cf && *dst == 0x00,
    };
}
void pdp11_op_sbc(Pdp11Cpu *const self, uint16_t *const dst) {
    *dst -= self->_stat.cf;
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*dst, 15),
        .zf = *dst == 0,
        .vf = self->_stat.cf && *dst == 0x7FFF,
        .cf = self->_stat.cf && *dst == 0xFFFF,
    };
}
void pdp11_op_sbcb(Pdp11Cpu *const self, uint8_t *const dst) {
    *dst -= self->_stat.cf;
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*dst, 7),
        .zf = *dst == 0,
        .vf = self->_stat.cf && *dst == 0x7F,
        .cf = self->_stat.cf && *dst == 0xFF,
    };
}

void pdp11_op_sxt(Pdp11Cpu *const self, uint16_t *const dst) {
    *dst = self->_stat.nf ? 0xFFFF : 0x0000;
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = self->_stat.nf,
        .zf = !self->_stat.nf,
        .vf = 0,
        .cf = self->_stat.cf,
    };
}

// rotates

void pdp11_op_ror(Pdp11Cpu *const self, uint16_t *const dst) {
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = self->_stat.cf,
        .cf = BIT(*dst, 0),
    };

    *dst = (*dst >> 1) | (self->_stat.cf << 15);

    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = self->_stat.nf,
        .zf = *dst == 0,
        .vf = self->_stat.nf ^ self->_stat.cf,
        .cf = self->_stat.cf,
    };
}
void pdp11_op_rorb(Pdp11Cpu *const self, uint8_t *const dst) {
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = self->_stat.cf,
        .cf = BIT(*dst, 0),
    };

    *dst = (*dst >> 1) | (self->_stat.cf << 7);

    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = self->_stat.nf,
        .zf = *dst == 0,
        .vf = self->_stat.nf ^ self->_stat.cf,
        .cf = self->_stat.cf,
    };
}
void pdp11_op_rol(Pdp11Cpu *const self, uint16_t *const dst) {
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = self->_stat.cf,  // TODO refactor this crappy line
        .cf = BIT(*dst, 15),
    };

    *dst = (*dst << 1) | self->_stat.nf;

    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*dst, 15),
        .zf = *dst == 0,
        .vf = self->_stat.nf ^ self->_stat.cf,
        .cf = self->_stat.cf,
    };
}
void pdp11_op_rolb(Pdp11Cpu *const self, uint8_t *const dst) {
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = self->_stat.cf,  // TODO refactor this crappy line
        .cf = BIT(*dst, 7),
    };

    *dst = (*dst << 1) | self->_stat.nf;

    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*dst, 7),
        .zf = *dst == 0,
        .vf = self->_stat.nf ^ self->_stat.cf,
        .cf = self->_stat.cf,
    };
}

void pdp11_op_swab(Pdp11Cpu *const self, uint16_t *const dst) {
    *dst = (uint16_t)(*dst << 8) | (uint8_t)(*dst >> 8);
    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(*dst, 7),
        .zf = (uint8_t)*dst == 0,
        .vf = 0,
        .cf = 0,
    };
}

// DUAL-OP

// arithmetic

void pdp11_op_mov(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
) {
    *dst = *src;
    pdp11_cpu_stat_set_flags_from_word(&self->_stat, *src);
}
void pdp11_op_movb(
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
    pdp11_cpu_stat_set_flags_from_byte(&self->_stat, *dst);
}
void pdp11_op_add(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
) {
    uint32_t const result = xword(*src) + xword(*dst);
    *dst = result;
    pdp11_cpu_stat_set_flags_from_xword(&self->_stat, result, false);
}
void pdp11_op_sub(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
) {
    uint32_t const result = xword(*src) + xword(-*dst);
    *dst = result;
    pdp11_cpu_stat_set_flags_from_xword(&self->_stat, result, true);
}
void pdp11_op_cmp(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t const *const dst
) {
    pdp11_cpu_stat_set_flags_from_xword(
        &self->_stat,
        xword(*src) + xword(-*dst),
        true
    );
}
void pdp11_op_cmpb(
    Pdp11Cpu *const self,
    uint8_t const *const src,
    uint8_t const *const dst
) {
    pdp11_cpu_stat_set_flags_from_xbyte(
        &self->_stat,
        xbyte(*src) + xbyte(-*dst),
        true
    );
}

// register destination

void pdp11_op_mul(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
) {
    uint32_t const result =
        (uint32_t)((int16_t)*src * (int16_t)pdp11_cpu_rx(self, r_i));
    if ((r_i & 1) == 0) pdp11_cpu_rx(self, r_i | 1) = (uint16_t)(result >> 16);
    pdp11_cpu_rx(self, r_i) = (uint16_t)result;

    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(result, 15),
        .zf = (uint16_t)result == 0,
        .vf = 0,
        .cf = (uint16_t)(result >> 16) != (BIT(result, 15) ? 0xFFFF : 0x0000),
    };
}
void pdp11_op_div(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
) {
    if (*src == 0) {
        self->_stat.vf = self->_stat.cf = 1;
        return;
    }

    uint32_t dst_value = pdp11_cpu_rx(self, r_i);
    if ((r_i & 1) == 0) dst_value |= pdp11_cpu_rx(self, r_i | 1) << 16;

    uint32_t const quotient = dst_value / *src;

    if ((uint16_t)(quotient >> 16) != 0) {
        self->_stat.vf = 1;
        return;
    }

    pdp11_cpu_rx(self, r_i) = quotient;
    if ((r_i & 1) == 0) pdp11_cpu_rx(self, r_i) = dst_value % *src;

    self->_stat = (Pdp11CpuStat){
        .priority = self->_stat.priority,
        .tf = self->_stat.tf,
        .nf = BIT(quotient, 15),
        .zf = quotient == 0,
        .vf = 0,
        .cf = 0,
    };
}

void pdp11_op_xor(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t *const dst
) {
    *dst ^= pdp11_cpu_rx(self, r_i);
    pdp11_cpu_stat_set_flags_from_word(&self->_stat, *dst);
}

// logical

void pdp11_op_bit(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t const *const dst
) {
    pdp11_cpu_stat_set_flags_from_word(&self->_stat, *dst & *src);
}
void pdp11_op_bitb(
    Pdp11Cpu *const self,
    uint8_t const *const src,
    uint8_t const *const dst
) {
    pdp11_cpu_stat_set_flags_from_byte(&self->_stat, *dst & *src);
}
void pdp11_op_bis(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
) {
    *dst |= *src;
    pdp11_op_bit(self, dst, dst);
}
void pdp11_op_bisb(
    Pdp11Cpu *const self,
    uint8_t const *const src,
    uint8_t *const dst
) {
    *dst |= *src;
    pdp11_op_bitb(self, dst, dst);
}
void pdp11_op_bic(
    Pdp11Cpu *const self,
    uint16_t const *const src,
    uint16_t *const dst
) {
    *dst &= ~*src;
    pdp11_op_bit(self, dst, dst);
}
void pdp11_op_bicb(
    Pdp11Cpu *const self,
    uint8_t const *const src,
    uint8_t *const dst
) {
    *dst &= ~*src;
    pdp11_op_bitb(self, dst, dst);
}

// PROGRAM CONTROL

// branches

void pdp11_op_br(Pdp11Cpu *const self, uint8_t const off) {
    pdp11_cpu_pc(self) += 2 * (int16_t)(int8_t)off;
}

void pdp11_op_bne_be(Pdp11Cpu *const self, bool const cond, uint8_t const off) {
    if (self->_stat.zf == cond) pdp11_op_br(self, off);
}
void pdp11_op_bpl_bmi(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
) {
    if (self->_stat.nf == cond) pdp11_op_br(self, off);
}
void pdp11_op_bcc_bcs(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
) {
    if (self->_stat.cf == cond) pdp11_op_br(self, off);
}
void pdp11_op_bvc_bvs(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
) {
    if (self->_stat.vf == cond) pdp11_op_br(self, off);
}

void pdp11_op_bge_bl(Pdp11Cpu *const self, bool const cond, uint8_t const off) {
    if ((self->_stat.nf ^ self->_stat.vf) == cond) pdp11_op_br(self, off);
}
void pdp11_op_bg_ble(Pdp11Cpu *const self, bool const cond, uint8_t const off) {
    if ((self->_stat.zf | (self->_stat.nf ^ self->_stat.vf)) == cond)
        pdp11_op_br(self, off);
}

void pdp11_op_bhi_blos(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
) {
    if ((self->_stat.cf | self->_stat.zf) == cond) pdp11_op_br(self, off);
}

// subroutine

void pdp11_op_jsr(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint16_t const *const src
) {
    pdp11_stack_push(self, pdp11_cpu_rx(self, r_i));
    pdp11_cpu_rx(self, r_i) = pdp11_cpu_pc(self);
    pdp11_cpu_pc(self) = *src;
}
void pdp11_op_mark(Pdp11Cpu *const self, unsigned const param_count) {
    pdp11_cpu_sp(self) = pdp11_cpu_pc(self) + 2 * param_count;
    pdp11_cpu_pc(self) = pdp11_cpu_rx(self, 5);
    pdp11_cpu_pc(self) = pdp11_stack_pop(self);
}
void pdp11_op_rts(Pdp11Cpu *const self, unsigned const r_i) {
    pdp11_cpu_pc(self) = pdp11_cpu_rx(self, r_i);
    pdp11_cpu_rx(self, r_i) = pdp11_stack_pop(self);
}

// program control

void pdp11_op_spl(Pdp11Cpu *const self, unsigned const value) {
    self->_stat.priority = value;
}

void pdp11_op_jmp(Pdp11Cpu *const self, uint16_t const *const src) {
    pdp11_cpu_pc(self) = *src;
}
void pdp11_op_sob(Pdp11Cpu *const self, unsigned const r_i, uint8_t const off) {
    if (--pdp11_cpu_rx(self, r_i) != 0) pdp11_cpu_pc(self) -= 2 * off;
}

// trap

void pdp11_op_emt(Pdp11Cpu *const self) { pdp11_request_intr(self, 030); }
void pdp11_op_trap(Pdp11Cpu *const self) { pdp11_request_intr(self, 034); }
void pdp11_op_bpt(Pdp11Cpu *const self) { pdp11_request_intr(self, 014); }
void pdp11_op_iot(Pdp11Cpu *const self) { pdp11_request_intr(self, 020); }
// TODO rti and rtt should be slightply different, but could not understand why
// at this point
void pdp11_op_rti(Pdp11Cpu *const self) {
    pdp11_cpu_pc(self) = pdp11_stack_pop(self);
    self->_stat = pdp11_cpu_stat(pdp11_stack_pop(self));
}
void pdp11_op_rtt(Pdp11Cpu *const self) {
    pdp11_cpu_pc(self) = pdp11_stack_pop(self);
    self->_stat = pdp11_cpu_stat(pdp11_stack_pop(self));
}

// MISC.

void pdp11_op_halt(Pdp11Cpu *const self) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_wait(Pdp11Cpu *const self) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_reset(Pdp11Cpu *const self) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}

void pdp11_op_mtpd(Pdp11Cpu *const self, uint16_t *const dst) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_mtpi(Pdp11Cpu *const self, uint16_t *const dst) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_mfpd(Pdp11Cpu *const self, uint16_t const *const src) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}
void pdp11_op_mfpi(Pdp11Cpu *const self, uint16_t const *const src) {
    printf("\tsorry, %s was not implemented (yet)\n", __func__);
}

// CONDITION CODES

void pdp11_op_clnzvc_senzvc(
    Pdp11Cpu *const self,
    bool const value,
    bool const do_affect_nf,
    bool const do_affect_zf,
    bool const do_affect_vf,
    bool const do_affect_cf
) {
    if (do_affect_nf) self->_stat.nf = value;
    if (do_affect_zf) self->_stat.zf = value;
    if (do_affect_vf) self->_stat.vf = value;
    if (do_affect_cf) self->_stat.cf = value;
}
