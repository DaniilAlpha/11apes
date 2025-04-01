#include "pdp11/cpu/pdp11_cpu.h"

#include <stddef.h>

#include "pdp11/cpu/pdp11_cpu_instr_set.h"

// TODO! Both JMP and JSR, used in address mode 2 (autoincrement), increment
// the register before using it as an address. This is a special case. and is
// not true of any other instruction

// TODO properly implement memory errors with traps: stack overflow, bus error,
// and also illegal instructions

/*************
 ** private **
 *************/

static uint16_t *pdp11_address_word(Pdp11Cpu *const self, unsigned const mode) {
    unsigned const r_i = BITS(mode, 0, 2);

    switch (BITS(mode, 3, 5)) {
    case 00: return &pdp11_cpu_rx(self, r_i);
    case 01: return pdp11_ram_word(self->_ram, pdp11_cpu_rx(self, r_i));
    case 02: {
        uint16_t *const result =
            pdp11_ram_word(self->_ram, pdp11_cpu_rx(self, r_i));
        pdp11_cpu_rx(self, r_i) += 2;
        return result;
    } break;
    case 03: {
        uint16_t *const result = pdp11_ram_word(
            self->_ram,
            *pdp11_ram_word(self->_ram, pdp11_cpu_rx(self, r_i))
        );
        pdp11_cpu_rx(self, r_i) += 2;
        return result;
    } break;
    case 04: return pdp11_ram_word(self->_ram, pdp11_cpu_rx(self, r_i) -= 2);
    case 05:
        return pdp11_ram_word(
            self->_ram,
            *pdp11_ram_word(self->_ram, pdp11_cpu_rx(self, r_i) -= 2)
        );
    case 06: {
        uint16_t const reg_val = pdp11_cpu_rx(self, r_i);
        return pdp11_ram_word(self->_ram, reg_val + pdp11_instr_next(self));
    } break;
    case 07: {
        uint16_t const reg_val = pdp11_cpu_rx(self, r_i);
        return pdp11_ram_word(
            self->_ram,
            *pdp11_ram_word(self->_ram, reg_val + pdp11_instr_next(self))
        );
    } break;
    }

    return NULL;
}
static uint8_t *pdp11_address_byte(Pdp11Cpu *const self, unsigned const mode) {
    unsigned const r_i = BITS(mode, 0, 2);

    switch (BITS(mode, 3, 5)) {
    case 00: return &pdp11_cpu_rl(self, r_i);
    case 01: return pdp11_ram_byte(self->_ram, pdp11_cpu_rx(self, r_i));
    case 02: {
        uint8_t *const result =
            pdp11_ram_byte(self->_ram, pdp11_cpu_rx(self, r_i));
        pdp11_cpu_rx(self, r_i) += r_i >= 06 ? 2 : 1;
        return result;
    } break;
    case 03: {
        uint8_t *const result = pdp11_ram_byte(
            self->_ram,
            *pdp11_ram_word(self->_ram, pdp11_cpu_rx(self, r_i))
        );
        pdp11_cpu_rx(self, r_i) += 2;
        return result;
    } break;
    case 04:
        return pdp11_ram_byte(
            self->_ram,
            pdp11_cpu_rx(self, r_i) -= r_i >= 06 ? 2 : 1
        );
    case 05:
        return pdp11_ram_byte(
            self->_ram,
            *pdp11_ram_word(self->_ram, pdp11_cpu_rx(self, r_i) -= 2)
        );
    case 06: {
        uint16_t const reg_val = pdp11_cpu_rx(self, r_i);
        return pdp11_ram_byte(self->_ram, reg_val + pdp11_instr_next(self));
    } break;
    case 07: {
        uint16_t const reg_val = pdp11_cpu_rx(self, r_i);
        return pdp11_ram_byte(
            self->_ram,
            *pdp11_ram_word(self->_ram, reg_val + pdp11_instr_next(self))
        );
    } break;
    }

    return NULL;
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
    self->_stat = stat;

    self->_ram = ram;
}
void pdp11_cpu_uninit(Pdp11Cpu *const self) {
    pdp11_cpu_pc(self) = 0;
    self->_stat = (Pdp11CpuStat){0};
}

void pdp11_cpu_exec_instr(Pdp11Cpu *const self, const uint16_t instr) {
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

    case 0104: return op_8 ? pdp11_op_emt(self) : pdp11_op_trap(self);
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
            BIT(instr, 4),
            BIT(instr, 3),
            BIT(instr, 2),
            BIT(instr, 1),
            BIT(instr, 0)
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

    // TODO send Reserved Instruction Trap
    return;
}
