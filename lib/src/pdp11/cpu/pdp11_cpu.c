#include "pdp11/cpu/pdp11_cpu.h"

#include <pthread.h>
#include <stdalign.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>

#include <assert.h>
#include <unistd.h>

#include "conviniences.h"
#include "pdp11/cpu/pdp11_cpu_instr.h"

#undef pdp11_cpu_rx
#define pdp11_cpu_rx(SELF_, I_) (*(uint16_t *)pdp11_cpu_rx((SELF_), (I_)))
static inline uint8_t *pdp11_cpu_rl(Pdp11Cpu *const self, unsigned const i) {
    return (uint8_t *)(self->_r + i);
}
#define pdp11_cpu_rl(SELF_, I_) (*(uint8_t *)pdp11_cpu_rl((SELF_), (I_)))
#undef pdp11_cpu_pc
#define pdp11_cpu_pc(SELF_) pdp11_cpu_rx((SELF_), 7)
#undef pdp11_cpu_sp
#define pdp11_cpu_sp(SELF_) pdp11_cpu_rx((SELF_), 6)

/***********************
 ** a word and a byte **
 ***********************/

typedef struct Pdp11Word Pdp11Word;
typedef struct Pdp11WordVtbl {
    uint16_t (*const read)(Pdp11Word const *const self);
    void (*const write)(Pdp11Word const *const self, uint16_t const value);
} Pdp11WordVtbl;
struct Pdp11Word {
    uint16_t const addr;
    void *const owner;

    Pdp11WordVtbl const *const vtbl;
};

static uint16_t pdp11_cpu_reg_word_read(Pdp11Word const *const self) {
    return pdp11_cpu_rx((Pdp11Cpu *)self->owner, self->addr);
}
static void
pdp11_cpu_reg_word_write(Pdp11Word const *const self, uint16_t const value) {
    pdp11_cpu_rx((Pdp11Cpu *)self->owner, self->addr) = value;
}
static Pdp11Word
pdp11_word_from_cpu_reg(Pdp11Cpu *const cpu, uint16_t const r_i) {
    static Pdp11WordVtbl const vtbl = {
        .read = pdp11_cpu_reg_word_read,
        .write = pdp11_cpu_reg_word_write,
    };
    return (Pdp11Word){.addr = r_i, .owner = cpu, .vtbl = &vtbl};
}

static uint16_t pdp11_unibus_word_read(Pdp11Word const *const self) {
    uint16_t data;
    return unibus_cpu_dati(self->owner, self->addr, &data), data;
}
static void
pdp11_unibus_word_write(Pdp11Word const *const self, uint16_t const value) {
    unibus_cpu_dato(self->owner, self->addr, value);
}
static Pdp11Word
pdp11_word_from_unibus(Unibus *const unibus, uint16_t const addr) {
    static Pdp11WordVtbl const vtbl = {
        .read = pdp11_unibus_word_read,
        .write = pdp11_unibus_word_write,
    };
    uint16_t _;
    if (unibus_cpu_dati(unibus, addr, &_) != Ok) return (Pdp11Word){0};
    return (Pdp11Word){.addr = addr, .owner = unibus, .vtbl = &vtbl};
}

typedef struct Pdp11Byte Pdp11Byte;
typedef struct Pdp11ByteVtbl {
    uint8_t (*const read)(Pdp11Byte const *const self);
    void (*const write)(Pdp11Byte const *const self, uint8_t const value);
} Pdp11ByteVtbl;
struct Pdp11Byte {
    uint16_t const addr;
    void *const owner;

    Pdp11ByteVtbl const *const vtbl;
};

static uint8_t pdp11_cpu_reg_byte_read(Pdp11Byte const *const self) {
    return pdp11_cpu_rl((Pdp11Cpu *)self->owner, self->addr);
}
static void
pdp11_cpu_reg_byte_write(Pdp11Byte const *const self, uint8_t const value) {
    pdp11_cpu_rl((Pdp11Cpu *)self->owner, self->addr) = value;
}
static Pdp11ByteVtbl const pdp11_cpu_reg_byte_vtbl = {
    .read = pdp11_cpu_reg_byte_read,
    .write = pdp11_cpu_reg_byte_write,
};
static Pdp11Byte
pdp11_byte_from_cpu_reg(Pdp11Cpu *const cpu, uint16_t const r_i) {
    return (Pdp11Byte
    ){.addr = r_i, .owner = cpu, .vtbl = &pdp11_cpu_reg_byte_vtbl};
}

static uint8_t pdp11_unibus_byte_read(Pdp11Byte const *const self) {
    uint16_t data;
    unibus_cpu_dati(self->owner, self->addr & ~(uint16_t)1, &data);
    return self->addr & 1 ? (uint8_t)(data >> 8) : (uint8_t)data;
}
static void
pdp11_unibus_byte_write(Pdp11Byte const *const self, uint8_t const value) {
    unibus_cpu_datob(self->owner, self->addr, value);
}
static Pdp11Byte
pdp11_byte_from_unibus(Unibus *const unibus, uint16_t const addr) {
    static Pdp11ByteVtbl const vtbl = {
        .read = pdp11_unibus_byte_read,
        .write = pdp11_unibus_byte_write,
    };
    uint16_t _;
    if (unibus_cpu_dati(unibus, addr & ~(uint16_t)1, &_) != Ok)
        return (Pdp11Byte){0};
    return (Pdp11Byte){.addr = addr, .owner = unibus, .vtbl = &vtbl};
}

/****************
 ** instr decl **
 ****************/

// SINGLE-OP

// general

forceinline void pdp11_cpu_instr_clr(Pdp11Cpu *const self, Pdp11Word const dst);
forceinline void
pdp11_cpu_instr_clrb(Pdp11Cpu *const self, Pdp11Byte const dst);
forceinline void pdp11_cpu_instr_inc(Pdp11Cpu *const self, Pdp11Word const dst);
forceinline void
pdp11_cpu_instr_incb(Pdp11Cpu *const self, Pdp11Byte const dst);
forceinline void pdp11_cpu_instr_dec(Pdp11Cpu *const self, Pdp11Word const dst);
forceinline void
pdp11_cpu_instr_decb(Pdp11Cpu *const self, Pdp11Byte const dst);

forceinline void pdp11_cpu_instr_neg(Pdp11Cpu *const self, Pdp11Word const dst);
forceinline void
pdp11_cpu_instr_negb(Pdp11Cpu *const self, Pdp11Byte const dst);

forceinline void pdp11_cpu_instr_tst(Pdp11Cpu *const self, Pdp11Word const src);
forceinline void
pdp11_cpu_instr_tstb(Pdp11Cpu *const self, Pdp11Byte const src);

// NOTE this is a `COMplement` instruction, like `not` in intel
forceinline void pdp11_cpu_instr_com(Pdp11Cpu *const self, Pdp11Word const dst);
forceinline void
pdp11_cpu_instr_comb(Pdp11Cpu *const self, Pdp11Byte const dst);

// shifts

forceinline void pdp11_cpu_instr_asr(Pdp11Cpu *const self, Pdp11Word const dst);
forceinline void
pdp11_cpu_instr_asrb(Pdp11Cpu *const self, Pdp11Byte const dst);
forceinline void pdp11_cpu_instr_asl(Pdp11Cpu *const self, Pdp11Word const dst);
forceinline void
pdp11_cpu_instr_aslb(Pdp11Cpu *const self, Pdp11Byte const dst);

forceinline void pdp11_cpu_instr_ash(
    Pdp11Cpu *const self,
    unsigned const r_i,
    Pdp11Byte const src
);
forceinline void pdp11_cpu_instr_ashc(
    Pdp11Cpu *const self,
    unsigned const r_i,
    Pdp11Word const src
);

// multiple-percision

forceinline void pdp11_cpu_instr_adc(Pdp11Cpu *const self, Pdp11Word const dst);
forceinline void
pdp11_cpu_instr_adcb(Pdp11Cpu *const self, Pdp11Byte const dst);
forceinline void pdp11_cpu_instr_sbc(Pdp11Cpu *const self, Pdp11Word const dst);
forceinline void
pdp11_cpu_instr_sbcb(Pdp11Cpu *const self, Pdp11Byte const dst);

forceinline void pdp11_cpu_instr_sxt(Pdp11Cpu *const self, Pdp11Word const dst);

// rotates

forceinline void pdp11_cpu_instr_ror(Pdp11Cpu *const self, Pdp11Word const dst);
forceinline void
pdp11_cpu_instr_rorb(Pdp11Cpu *const self, Pdp11Byte const dst);
forceinline void pdp11_cpu_instr_rol(Pdp11Cpu *const self, Pdp11Word const dst);
forceinline void
pdp11_cpu_instr_rolb(Pdp11Cpu *const self, Pdp11Byte const dst);

forceinline void
pdp11_cpu_instr_swab(Pdp11Cpu *const self, Pdp11Word const dst);

// DUAL-OP

// arithmetic

forceinline void pdp11_cpu_instr_mov(
    Pdp11Cpu *const self,
    Pdp11Word const src,
    Pdp11Word const dst
);
forceinline void pdp11_cpu_instr_movb(
    Pdp11Cpu *const self,
    Pdp11Byte const src,
    Pdp11Byte const dst
);
forceinline void pdp11_cpu_instr_add(
    Pdp11Cpu *const self,
    Pdp11Word const src,
    Pdp11Word const dst
);
forceinline void pdp11_cpu_instr_sub(
    Pdp11Cpu *const self,
    Pdp11Word const src,
    Pdp11Word const dst
);
forceinline void pdp11_cpu_instr_cmp(
    Pdp11Cpu *const self,
    Pdp11Word const src,
    Pdp11Word const dst
);
forceinline void pdp11_cpu_instr_cmpb(
    Pdp11Cpu *const self,
    Pdp11Byte const src,
    Pdp11Byte const dst
);

// register destination

forceinline void pdp11_cpu_instr_mul(
    Pdp11Cpu *const self,
    unsigned const r_i,
    Pdp11Word const src
);
forceinline void pdp11_cpu_instr_div(
    Pdp11Cpu *const self,
    unsigned const r_i,
    Pdp11Word const src
);

forceinline void pdp11_cpu_instr_xor(
    Pdp11Cpu *const self,
    unsigned const r_i,
    Pdp11Word const dst
);

// logical

forceinline void pdp11_cpu_instr_bit(
    Pdp11Cpu *const self,
    Pdp11Word const src,
    Pdp11Word const dst
);
forceinline void pdp11_cpu_instr_bitb(
    Pdp11Cpu *const self,
    Pdp11Byte const src,
    Pdp11Byte const dst
);
forceinline void pdp11_cpu_instr_bis(
    Pdp11Cpu *const self,
    Pdp11Word const src,
    Pdp11Word const dst
);
forceinline void pdp11_cpu_instr_bisb(
    Pdp11Cpu *const self,
    Pdp11Byte const src,
    Pdp11Byte const dst
);
forceinline void pdp11_cpu_instr_bic(
    Pdp11Cpu *const self,
    Pdp11Word const src,
    Pdp11Word const dst
);
forceinline void pdp11_cpu_instr_bicb(
    Pdp11Cpu *const self,
    Pdp11Byte const src,
    Pdp11Byte const dst
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

forceinline void pdp11_cpu_instr_jsr_jmp(
    Pdp11Cpu *const self,
    unsigned const r_i,
    unsigned const mode
);
forceinline void
pdp11_cpu_instr_mark(Pdp11Cpu *const self, unsigned const param_count);
forceinline void pdp11_cpu_instr_rts(Pdp11Cpu *const self, unsigned const r_i);

// program control

forceinline void pdp11_cpu_instr_spl(Pdp11Cpu *const self, uint8_t const value);

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
forceinline void pdp11_cpu_instr_rti_rtt(Pdp11Cpu *const self);

// MISC.

forceinline void pdp11_cpu_instr_halt(Pdp11Cpu *const self);
forceinline void pdp11_cpu_instr_wait(Pdp11Cpu *const self);
forceinline void pdp11_cpu_instr_reset(Pdp11Cpu *const self);
// NOTE `nop` is already implemented with `clnzvc`/`senzvc`

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

static Result pdp11_stack_push(Pdp11Cpu *const self, uint16_t const value) {
    pdp11_cpu_sp(self) -= 2;
    UNROLL(unibus_cpu_dato(self->_unibus, pdp11_cpu_sp(self), value));
    return Ok;
}
static Result pdp11_stack_pop(Pdp11Cpu *const self, uint16_t *const out) {
    UNROLL(unibus_cpu_dati(self->_unibus, pdp11_cpu_sp(self), out));
    pdp11_cpu_sp(self) += 2;
    return Ok;
}
static void pdp11_cpu_trap(Pdp11Cpu *const self, uint8_t const trap) {
    fprintf(stderr, "==TRAP== (%03o) \n", trap);
    fflush(stderr);
    uint16_t psw_word;
    if (pdp11_stack_push(self, pdp11_psw_to_word(&self->_psw)) != Ok ||
        pdp11_stack_push(self, pdp11_cpu_pc(self)) != Ok ||
        unibus_cpu_dati(self->_unibus, trap, &pdp11_cpu_pc(self)) != Ok ||
        unibus_cpu_dati(self->_unibus, trap + 2, &psw_word) != Ok)
        return pdp11_cpu_halt(self);

    pdp11_psw_set(&self->_psw, psw_word);
}

static void pdp11_cpu_service_intr(Pdp11Cpu *const self) {
    uint8_t const pending_intr =
        atomic_exchange(&self->__pending_intr, PDP11_CPU_NO_TRAP);
    if (pending_intr == PDP11_CPU_NO_TRAP) return;

    fprintf(stderr, "  (from intr)  ");
    pdp11_cpu_trap(self, pending_intr);

    sem_post(&self->__pending_intr_sem);
}

static uint16_t pdp11_cpu_fetch(Pdp11Cpu *const self) {
    uint16_t instr;
    if (unibus_cpu_dati(self->_unibus, pdp11_cpu_pc(self), &instr) != Ok) {
        pdp11_cpu_trap(self, PDP11_CPU_TRAP_CPU_ERR);
        if (unibus_cpu_dati(self->_unibus, pdp11_cpu_pc(self), &instr) != Ok)
            pdp11_cpu_halt(self);
    }
    pdp11_cpu_pc(self) += 2;
    return instr;
}

static Pdp11Word
pdp11_cpu_address_word(Pdp11Cpu *const self, unsigned const mode) {
    unsigned const r_i = BITS(mode, 0, 2);

    switch (BITS(mode, 3, 5)) {
    case 00: return pdp11_word_from_cpu_reg(self, r_i);
    case 01:
        return pdp11_word_from_unibus(self->_unibus, pdp11_cpu_rx(self, r_i));
    case 02: {
        Pdp11Word const word =
            pdp11_word_from_unibus(self->_unibus, pdp11_cpu_rx(self, r_i));
        pdp11_cpu_rx(self, r_i) += 2;
        return word;
    } break;
    case 03: {
        uint16_t addr;
        if (unibus_cpu_dati(self->_unibus, pdp11_cpu_rx(self, r_i), &addr) ==
            Ok) {
            pdp11_cpu_rx(self, r_i) += 2;
            return pdp11_word_from_unibus(self->_unibus, addr);
        }
    } break;
    case 04: {
        pdp11_cpu_rx(self, r_i) -= 2;
        return pdp11_word_from_unibus(self->_unibus, pdp11_cpu_rx(self, r_i));
    } break;
    case 05: {
        pdp11_cpu_rx(self, r_i) -= 2;
        uint16_t addr;
        if (unibus_cpu_dati(self->_unibus, pdp11_cpu_rx(self, r_i), &addr) ==
            Ok)
            return pdp11_word_from_unibus(self->_unibus, addr);
    } break;
    case 06: {
        uint16_t off;
        if (unibus_cpu_dati(self->_unibus, pdp11_cpu_pc(self), &off) == Ok) {
            pdp11_cpu_pc(self) += 2;
            return pdp11_word_from_unibus(
                self->_unibus,
                off + pdp11_cpu_rx(self, r_i)
            );
        }
    } break;
    case 07: {
        uint16_t off, addr;
        if (unibus_cpu_dati(self->_unibus, pdp11_cpu_pc(self), &off) == Ok) {
            pdp11_cpu_pc(self) += 2;
            if (unibus_cpu_dati(
                    self->_unibus,
                    off + pdp11_cpu_rx(self, r_i),
                    &addr
                ) == Ok)
                return pdp11_word_from_unibus(self->_unibus, addr);
        }
    } break;
    default: assert(false);
    }

    return (Pdp11Word){.vtbl = NULL};
}
static Pdp11Byte
pdp11_cpu_address_byte(Pdp11Cpu *const self, unsigned const mode) {
    unsigned const r_i = BITS(mode, 0, 2);

    switch (BITS(mode, 3, 5)) {
    case 00: return pdp11_byte_from_cpu_reg(self, r_i);
    case 01:
        return pdp11_byte_from_unibus(self->_unibus, pdp11_cpu_rx(self, r_i));
    case 02: {
        Pdp11Byte const byte =
            pdp11_byte_from_unibus(self->_unibus, pdp11_cpu_rx(self, r_i));
        pdp11_cpu_rx(self, r_i) += r_i >= 06 ? 2 : 1;
        return byte;
    } break;
    case 03: {
        uint16_t addr;
        if (unibus_cpu_dati(self->_unibus, pdp11_cpu_rx(self, r_i), &addr) ==
            Ok) {
            pdp11_cpu_rx(self, r_i) += 2;
            return pdp11_byte_from_unibus(self->_unibus, addr);
        }
    } break;
    case 04: {
        pdp11_cpu_rx(self, r_i) -= r_i >= 06 ? 2 : 1;
        return pdp11_byte_from_unibus(self->_unibus, pdp11_cpu_rx(self, r_i));
    } break;
    case 05: {
        pdp11_cpu_rx(self, r_i) -= 2;
        uint16_t addr;
        if (unibus_cpu_dati(self->_unibus, pdp11_cpu_rx(self, r_i), &addr) ==
            Ok)
            return pdp11_byte_from_unibus(self->_unibus, addr);
    } break;
    case 06: {
        uint16_t off;
        if (unibus_cpu_dati(self->_unibus, pdp11_cpu_pc(self), &off) == Ok) {
            pdp11_cpu_pc(self) += 2;
            return pdp11_byte_from_unibus(
                self->_unibus,
                off + pdp11_cpu_rx(self, r_i)
            );
        }
    } break;
    case 07: {
        uint16_t off, addr;
        if (unibus_cpu_dati(self->_unibus, pdp11_cpu_pc(self), &off) == Ok) {
            pdp11_cpu_pc(self) += 2;
            if (unibus_cpu_dati(
                    self->_unibus,
                    off + pdp11_cpu_rx(self, r_i),
                    &addr
                ) == Ok)
                return pdp11_byte_from_unibus(self->_unibus, addr);
        }
    } break;
    default: assert(false);
    }

    return (Pdp11Byte){.vtbl = NULL};
}
static inline uint16_t
pdp11_cpu_jmp_jsr_effective_addr(Pdp11Cpu *const self, unsigned const mode) {
    unsigned const r_i = BITS(mode, 0, 2);

    switch (BITS(mode, 3, 5)) {
    case 00: break;
    case 01: return pdp11_cpu_rx(self, r_i);
    case 02: return pdp11_cpu_rx(self, r_i) += 2;
    case 03: {
        uint16_t addr;
        if (unibus_cpu_dati(self->_unibus, pdp11_cpu_rx(self, r_i), &addr) ==
            Ok) {
            pdp11_cpu_rx(self, r_i) += 2;
            return addr;
        }
    } break;
    case 04: return pdp11_cpu_rx(self, r_i) -= 2;
    case 05: {
        pdp11_cpu_rx(self, r_i) -= 2;
        uint16_t addr;
        if (unibus_cpu_dati(self->_unibus, pdp11_cpu_rx(self, r_i), &addr) ==
            Ok)
            return addr;
    } break;
    case 06: {
        uint16_t off;
        if (unibus_cpu_dati(self->_unibus, pdp11_cpu_pc(self), &off) == Ok) {
            pdp11_cpu_pc(self) += 2;
            return off + pdp11_cpu_rx(self, r_i);
        }
    } break;
    case 07: {
        uint16_t off, addr;
        if (unibus_cpu_dati(self->_unibus, pdp11_cpu_pc(self), &off) == Ok) {
            pdp11_cpu_pc(self) += 2;
            if (unibus_cpu_dati(
                    self->_unibus,
                    off + pdp11_cpu_rx(self, r_i),
                    &addr
                ) == Ok)
                return addr;
        }
    } break;
    default: assert(false);
    }

    return pdp11_cpu_trap(self, PDP11_CPU_TRAP_CPU_ERR), 0;
}
static void pdp11_cpu_exec_oo(Pdp11Cpu *const self, Pdp11CpuInstr const instr) {
    if (instr.u.oo.opcode & 010 && instr.u.oo.opcode != 016) {
        Pdp11Byte const o0 = pdp11_cpu_address_byte(self, instr.u.oo.o0);
        if (!o0.vtbl) return pdp11_cpu_trap(self, PDP11_CPU_TRAP_CPU_ERR);
        Pdp11Byte const o1 = pdp11_cpu_address_byte(self, instr.u.oo.o1);
        if (!o1.vtbl) return pdp11_cpu_trap(self, PDP11_CPU_TRAP_CPU_ERR);
        switch (instr.u.oo.opcode) {
        case 011: return pdp11_cpu_instr_movb(self, o0, o1);
        case 012: return pdp11_cpu_instr_cmpb(self, o0, o1);
        case 013: return pdp11_cpu_instr_bitb(self, o0, o1);
        case 014: return pdp11_cpu_instr_bicb(self, o0, o1);
        case 015: return pdp11_cpu_instr_bisb(self, o0, o1);
        }
    } else {
        Pdp11Word const o0 = pdp11_cpu_address_word(self, instr.u.oo.o0);
        if (!o0.vtbl) return pdp11_cpu_trap(self, PDP11_CPU_TRAP_CPU_ERR);
        Pdp11Word const o1 = pdp11_cpu_address_word(self, instr.u.oo.o1);
        if (!o1.vtbl) return pdp11_cpu_trap(self, PDP11_CPU_TRAP_CPU_ERR);
        switch (instr.u.oo.opcode) {
        case 001: return pdp11_cpu_instr_mov(self, o0, o1);
        case 002: return pdp11_cpu_instr_cmp(self, o0, o1);
        case 003: return pdp11_cpu_instr_bit(self, o0, o1);
        case 004: return pdp11_cpu_instr_bic(self, o0, o1);
        case 005: return pdp11_cpu_instr_bis(self, o0, o1);
        case 006: return pdp11_cpu_instr_add(self, o0, o1);
        case 016: return pdp11_cpu_instr_sub(self, o0, o1);
        }
    }
}
static void pdp11_cpu_exec_ro(Pdp11Cpu *const self, Pdp11CpuInstr const instr) {
    if (instr.u.ro.opcode == 0072) {
        uint16_t const r = instr.u.ro.r;
        Pdp11Byte const o = pdp11_cpu_address_byte(self, instr.u.ro.o);
        if (!o.vtbl) return pdp11_cpu_trap(self, PDP11_CPU_TRAP_CPU_ERR);
        return pdp11_cpu_instr_ash(self, r, o);
    } else {
        uint16_t const r = instr.u.ro.r;
        Pdp11Word const o = pdp11_cpu_address_word(self, instr.u.ro.o);
        if (!o.vtbl) return pdp11_cpu_trap(self, PDP11_CPU_TRAP_CPU_ERR);
        switch (instr.u.ro.opcode) {
        case 0070: return pdp11_cpu_instr_mul(self, r, o);
        case 0071: return pdp11_cpu_instr_div(self, r, o);
        case 0073: return pdp11_cpu_instr_ashc(self, r, o);
        case 0074: return pdp11_cpu_instr_xor(self, r, o);
        }
    }
}
static void
pdp11_cpu_exec_branch(Pdp11Cpu *const self, Pdp11CpuInstr const instr) {
    uint16_t const cond = instr.u.branch.cond;
    uint16_t const off = instr.u.branch.off;
    switch (instr.u.branch.opcode) {
    case 0000: return assert(cond == 1), pdp11_cpu_instr_br(self, off);
    case 0001: return pdp11_cpu_instr_bne_be(self, cond, off);
    case 0002: return pdp11_cpu_instr_bge_bl(self, cond, off);
    case 0003: return pdp11_cpu_instr_bg_ble(self, cond, off);
    case 0100: return pdp11_cpu_instr_bpl_bmi(self, cond, off);
    case 0101: return pdp11_cpu_instr_bhi_blos(self, cond, off);
    case 0102: return pdp11_cpu_instr_bvc_bvs(self, cond, off);
    case 0103: return pdp11_cpu_instr_bcc_bcs(self, cond, off);
    }
}
static void pdp11_cpu_exec_o(Pdp11Cpu *const self, Pdp11CpuInstr const instr) {
    if (instr.u.o.opcode & 01000) {
        Pdp11Byte const o = pdp11_cpu_address_byte(self, instr.u.o.o);
        if (!o.vtbl) return pdp11_cpu_trap(self, PDP11_CPU_TRAP_CPU_ERR);
        switch (instr.u.o.opcode) {
        case 01050: return pdp11_cpu_instr_clrb(self, o);
        case 01051: return pdp11_cpu_instr_comb(self, o);
        case 01052: return pdp11_cpu_instr_incb(self, o);
        case 01053: return pdp11_cpu_instr_decb(self, o);
        case 01055: return pdp11_cpu_instr_adcb(self, o);
        case 01056: return pdp11_cpu_instr_sbcb(self, o);
        case 01054: return pdp11_cpu_instr_negb(self, o);
        case 01057: return pdp11_cpu_instr_tstb(self, o);
        case 01060: return pdp11_cpu_instr_rorb(self, o);
        case 01061: return pdp11_cpu_instr_rolb(self, o);
        case 01062: return pdp11_cpu_instr_asrb(self, o);
        case 01063: return pdp11_cpu_instr_aslb(self, o);
        }
    } else {
        Pdp11Word const o = pdp11_cpu_address_word(self, instr.u.o.o);
        if (!o.vtbl) return pdp11_cpu_trap(self, PDP11_CPU_TRAP_CPU_ERR);
        switch (instr.u.o.opcode) {
        case 00003: return pdp11_cpu_instr_swab(self, o);
        case 00050: return pdp11_cpu_instr_clr(self, o);
        case 00051: return pdp11_cpu_instr_com(self, o);
        case 00052: return pdp11_cpu_instr_inc(self, o);
        case 00053: return pdp11_cpu_instr_dec(self, o);
        case 00054: return pdp11_cpu_instr_neg(self, o);
        case 00055: return pdp11_cpu_instr_adc(self, o);
        case 00056: return pdp11_cpu_instr_sbc(self, o);
        case 00057: return pdp11_cpu_instr_tst(self, o);
        case 00060: return pdp11_cpu_instr_ror(self, o);
        case 00061: return pdp11_cpu_instr_rol(self, o);
        case 00062: return pdp11_cpu_instr_asr(self, o);
        case 00063: return pdp11_cpu_instr_asl(self, o);
        case 00067: return pdp11_cpu_instr_sxt(self, o);
        }
    }
}
static void pdp11_cpu_exec_r(Pdp11Cpu *const self, Pdp11CpuInstr const instr) {
    uint16_t const opcode = instr.u.r.opcode;
    uint16_t const r = instr.u.r.r;
    switch (opcode) {
    case 000020: return pdp11_cpu_instr_rts(self, r);
    case 000023: return pdp11_cpu_instr_spl(self, r);
    }
}
static void
pdp11_cpu_exec_misc(Pdp11Cpu *const self, Pdp11CpuInstr const instr) {
    uint16_t const opcode = instr.u.misc.opcode;
    switch (opcode) {
    case 0104000 ... 0104377: return pdp11_cpu_instr_emt(self);
    case 0104400 ... 0104777: return pdp11_cpu_instr_trap(self);

    case 0000240 ... 0000277:
        return pdp11_cpu_instr_clnzvc_senzvc(
            self,
            BIT(opcode, 4),
            BIT(opcode, 3),
            BIT(opcode, 2),
            BIT(opcode, 1),
            BIT(opcode, 0)
        );

    case 0006400 ... 0006477:
        return pdp11_cpu_instr_mark(self, BITS(opcode, 0, 5));

    case 0000006:
    case 0000002: return pdp11_cpu_instr_rti_rtt(self);
    case 0000003: return pdp11_cpu_instr_bpt(self);
    case 0000004: return pdp11_cpu_instr_iot(self);

    case 0000000: return pdp11_cpu_instr_halt(self);
    case 0000001: return pdp11_cpu_instr_wait(self);
    case 0000005: return pdp11_cpu_instr_reset(self);
    }
}

static void pdp11_cpu_thread_helper(Pdp11Cpu *const self) {
    bool should_trap = false;
    while (self->__should_thread_run) {
        while (self->__should_thread_run &&
               (self->_state == PDP11_CPU_STATE_HALT ||
                self->_state == PDP11_CPU_STATE_WAIT))
            sleep(0);
        if (!self->__should_thread_run) break;

        if (should_trap) pdp11_cpu_trap(self, PDP11_CPU_TRAP_BPT);
        should_trap = self->_psw.flags.t;

        uint16_t const encoded = pdp11_cpu_fetch(self);
        fprintf(
            stderr,
            "exec at %06o : %06o\n",
            pdp11_cpu_pc(self) - 2,
            encoded
        ),
            fflush(stderr);

        // TODO temporary breakpoint for debugging
        // if (pdp11_cpu_pc(self) - 2 == 0006070 ||
        //     pdp11_cpu_pc(self) - 2 == 0006130)
        //     self->_state = PDP11_CPU_STATE_HALT;

        Pdp11CpuInstr const instr = pdp11_cpu_instr(encoded);

        uint16_t const next_pc = pdp11_cpu_pc(self);
        switch (instr.type) {
        case PDP11_CPU_INSTR_TYPE_OO: pdp11_cpu_exec_oo(self, instr); break;
        case PDP11_CPU_INSTR_TYPE_RO: pdp11_cpu_exec_ro(self, instr); break;
        case PDP11_CPU_INSTR_TYPE_O: pdp11_cpu_exec_o(self, instr); break;
        case PDP11_CPU_INSTR_TYPE_BRANCH:
            pdp11_cpu_exec_branch(self, instr);
            break;
        case PDP11_CPU_INSTR_TYPE_R: pdp11_cpu_exec_r(self, instr); break;
        case PDP11_CPU_INSTR_TYPE_SOB:
            pdp11_cpu_instr_sob(self, instr.u.sob.r, instr.u.sob.off);
            break;
        case PDP11_CPU_INSTR_TYPE_JSR:
            pdp11_cpu_instr_jsr_jmp(self, instr.u.jsr.r, instr.u.jsr.o);
            break;
        case PDP11_CPU_INSTR_TYPE_JMP:
            pdp11_cpu_instr_jsr_jmp(self, -1, instr.u.jmp.o);
            break;
        case PDP11_CPU_INSTR_TYPE_MISC: pdp11_cpu_exec_misc(self, instr); break;
        case PDP11_CPU_INSTR_TYPE_RESERVED:
            pdp11_cpu_trap(self, PDP11_CPU_TRAP_RESERVED_INSTR);
            break;
        }

        if ((uint16_t)(pdp11_cpu_pc(self) - next_pc) > 4)
            fprintf(stderr, "\n"), fflush(stderr);

        if (self->_state != PDP11_CPU_STATE_HALT &&
            self->_state != PDP11_CPU_STATE_WAIT)
            pdp11_cpu_service_intr(self);

        if (self->_state == PDP11_CPU_STATE_STEP)
            self->_state = PDP11_CPU_STATE_HALT;

        usleep(1);
    }
}
static void *pdp11_cpu_thread(void *const vself) {
    return pdp11_cpu_thread_helper(vself), NULL;
};

/************
 ** public **
 ************/

Result pdp11_cpu_init(Pdp11Cpu *const self, Unibus *const unibus) {
    for (unsigned i = 0; i < PDP11_CPU_REG_COUNT; i++)
        pdp11_cpu_rx(self, i) = 0;
    UNROLL(pdp11_psw_init(&self->_psw));

    self->_unibus = unibus;

    sem_init(&self->__pending_intr_sem, false, 1);
    self->__pending_intr = PDP11_CPU_NO_TRAP;

    self->_state = PDP11_CPU_STATE_HALT;

    self->__should_thread_run = true;
    if (pthread_create(&self->_thread, NULL, pdp11_cpu_thread, self) != 0)
        return UnknownErr;

    return Ok;
}
void pdp11_cpu_uninit(Pdp11Cpu *const self) {
    self->__should_thread_run = false;
    pthread_join(self->_thread, NULL);

    sem_destroy(&self->__pending_intr_sem);

    pdp11_psw_uninit(&self->_psw);
    pdp11_cpu_pc(self) = 0;
}
void pdp11_cpu_reset(Pdp11Cpu *const self) {
    // for (unsigned i = 0; i < PDP11_CPU_REG_COUNT - 1; i++)
    //     pdp11_cpu_rx(self, i) = 0;
    self->_psw = (Pdp11Psw){0};
    atomic_exchange(&self->__pending_intr, PDP11_CPU_NO_TRAP);
}

void pdp11_cpu_intr(Pdp11Cpu *const self, uint8_t const intr) {
    sem_wait(&self->__pending_intr_sem);
    uint8_t const old_intr = atomic_exchange(&self->__pending_intr, intr);
    // TODO this assumes no two interrupts can happen at the same time, which
    // is defenetely false, just a bit unlikely
    assert(old_intr == PDP11_CPU_NO_TRAP), (void)old_intr;

    // TODO this can happen right before the `wait` instr is executed, so not an
    // ideal way
    if (self->_state == PDP11_CPU_STATE_WAIT)
        self->_state = PDP11_CPU_STATE_RUN;
}

void pdp11_cpu_halt(Pdp11Cpu *const self) {
    self->_state = PDP11_CPU_STATE_HALT;
}
void pdp11_cpu_continue(Pdp11Cpu *const self) {
    self->_state = PDP11_CPU_STATE_RUN;
}
void pdp11_cpu_single_step(Pdp11Cpu *const self) {
    self->_state = PDP11_CPU_STATE_STEP;
}

/****************
 ** instr impl **
 ****************/

static inline uint32_t xword(uint16_t const word) {
    return (word & 0x8000 ? 0x10000 : 0x00000) | word;
}

static inline void
pdp11_psw_set_flags_from_word(Pdp11Psw *const self, uint16_t const value) {
    self->flags = (Pdp11PswFlags){
        .t = self->flags.t,
        .n = BIT(value, 15),
        .z = value == 0,
        .v = 0,
        .c = self->flags.c,
    };
}
static inline void
pdp11_psw_set_flags_from_byte(Pdp11Psw *const self, uint8_t const value) {
    self->flags = (Pdp11PswFlags){
        .t = self->flags.t,
        .n = BIT(value, 7),
        .z = value == 0,
        .v = 0,
        .c = self->flags.c,
    };
}

// SINGLE-OP

// general

void pdp11_cpu_instr_clr(Pdp11Cpu *const self, Pdp11Word const dst) {
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = 0,
        .z = 1,
        .v = 0,
        .c = 0,
    };
    dst.vtbl->write(&dst, 0);
}
void pdp11_cpu_instr_clrb(Pdp11Cpu *const self, Pdp11Byte const dst) {
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = 0,
        .z = 1,
        .v = 0,
        .c = 0,
    };
    dst.vtbl->write(&dst, 0);
}
void pdp11_cpu_instr_inc(Pdp11Cpu *const self, Pdp11Word const dst) {
    uint16_t const res = dst.vtbl->read(&dst) + 1;
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 15),
        .z = res == 0,
        .v = res == 0x8000,
        .c = self->_psw.flags.c,
    };
    dst.vtbl->write(&dst, res);
}
void pdp11_cpu_instr_incb(Pdp11Cpu *const self, Pdp11Byte const dst) {
    uint8_t const res = dst.vtbl->read(&dst) + 1;
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 7),
        .z = res == 0,
        .v = res == 0x80,
        .c = self->_psw.flags.c,
    };
    dst.vtbl->write(&dst, res);
}
void pdp11_cpu_instr_dec(Pdp11Cpu *const self, Pdp11Word const dst) {
    uint16_t const res = dst.vtbl->read(&dst) - 1;
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 15),
        .z = res == 0,
        .v = res == 0x7FFF,
        .c = self->_psw.flags.c,
    };
    dst.vtbl->write(&dst, res);
}
void pdp11_cpu_instr_decb(Pdp11Cpu *const self, Pdp11Byte const dst) {
    uint8_t const res = dst.vtbl->read(&dst) - 1;
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 7),
        .z = res == 0,
        .v = res == 0x7F,
        .c = self->_psw.flags.c,
    };
    dst.vtbl->write(&dst, res);
}

void pdp11_cpu_instr_neg(Pdp11Cpu *const self, Pdp11Word const dst) {
    uint16_t const res = -dst.vtbl->read(&dst);
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 15),
        .z = res == 0,
        .v = res == 0x8000,
        .c = res != 0,
    };
    dst.vtbl->write(&dst, res);
}
void pdp11_cpu_instr_negb(Pdp11Cpu *const self, Pdp11Byte const dst) {
    uint8_t const res = -dst.vtbl->read(&dst);
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 7),
        .z = res == 0,
        .v = res == 0x80,
        .c = res != 0,
    };
    dst.vtbl->write(&dst, res);
}

void pdp11_cpu_instr_tst(Pdp11Cpu *const self, Pdp11Word const src) {
    uint16_t const src_val = src.vtbl->read(&src);
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(src_val, 15),
        .z = src_val == 0,
        .v = 0,
        .c = 0,
    };
}
void pdp11_cpu_instr_tstb(Pdp11Cpu *const self, Pdp11Byte const src) {
    uint8_t const src_val = src.vtbl->read(&src);
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(src_val, 7),
        .z = src_val == 0,
        .v = 0,
        .c = 0,
    };
}

void pdp11_cpu_instr_com(Pdp11Cpu *const self, Pdp11Word const dst) {
    uint16_t const res = ~dst.vtbl->read(&dst);
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 15),
        .z = res == 0,
        .v = 0,
        .c = 1,
    };
    dst.vtbl->write(&dst, res);
}
void pdp11_cpu_instr_comb(Pdp11Cpu *const self, Pdp11Byte const dst) {
    uint8_t const res = ~dst.vtbl->read(&dst);
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 7),
        .z = res == 0,
        .v = 0,
        .c = 1,
    };
    dst.vtbl->write(&dst, res);
}

// shifts

void pdp11_cpu_instr_asr(Pdp11Cpu *const self, Pdp11Word const dst) {
    uint16_t const dst_val = dst.vtbl->read(&dst);
    uint16_t const res = (int16_t)dst_val >> 1;
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 15),
        .z = res == 0,
        .v = BIT(res, 15) ^ BIT(dst_val, 0),
        .c = BIT(dst_val, 0),
    };
    dst.vtbl->write(&dst, res);
}
void pdp11_cpu_instr_asrb(Pdp11Cpu *const self, Pdp11Byte const dst) {
    uint8_t const dst_val = dst.vtbl->read(&dst);
    uint8_t const res = (int8_t)dst_val >> 1;
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 7),
        .z = res == 0,
        .v = BIT(res, 7) ^ BIT(dst_val, 0),
        .c = BIT(dst_val, 0),
    };
    dst.vtbl->write(&dst, res);
}
void pdp11_cpu_instr_asl(Pdp11Cpu *const self, Pdp11Word const dst) {
    uint16_t const dst_val = dst.vtbl->read(&dst);
    uint16_t const res = dst_val << 1;
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 15),
        .z = res == 0,
        .v = BIT(res, 15) ^ BIT(dst_val, 15),
        .c = BIT(dst_val, 15),
    };
    dst.vtbl->write(&dst, res);
}
void pdp11_cpu_instr_aslb(Pdp11Cpu *const self, Pdp11Byte const dst) {
    uint8_t const dst_val = dst.vtbl->read(&dst);
    uint8_t const res = dst_val << 1;
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 7),
        .z = res == 0,
        .v = BIT(res, 7) ^ BIT(dst_val, 7),
        .c = BIT(dst_val, 7),
    };
    dst.vtbl->write(&dst, res);
}

void pdp11_cpu_instr_ash(
    Pdp11Cpu *const self,
    unsigned const r_i,
    Pdp11Byte const src
) {
    Pdp11Word const dst = pdp11_word_from_cpu_reg(self, r_i);

    uint16_t const dst_val = dst.vtbl->read(&dst);

    uint8_t const src_val = src.vtbl->read(&src);
    bool const do_shift_right = BIT(src_val, 5);
    uint8_t const shift_amount = BITS(src_val, 0, 4);

    uint16_t const res =
        // TODO? not sure if need to shift signed
        do_shift_right ? dst_val >> shift_amount : dst_val << shift_amount;

    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(dst_val, 15),
        .z = res == 0,
        .v = BIT(res, 15) ^ BIT(dst_val, 15),
        .c = BIT(dst_val, do_shift_right ? shift_amount : 16 - shift_amount),
    };
    dst.vtbl->write(&dst, res);
}
void pdp11_cpu_instr_ashc(
    Pdp11Cpu *const self,
    unsigned const r_i,
    Pdp11Word const src
) {
    Pdp11Word const dst0 = pdp11_word_from_cpu_reg(self, r_i),
                    dst1 = pdp11_word_from_cpu_reg(self, r_i | 1);

    uint32_t const dst_val =
        dst0.vtbl->read(&dst0) | (dst1.vtbl->read(&dst1) << 16);

    uint8_t const src_val = src.vtbl->read(&src);
    bool const do_shift_right = BIT(src_val, 5);
    uint8_t const shift_amount = BITS(src_val, 0, 4);

    // TODO? not sure if need to shift signed
    uint32_t const res =
        do_shift_right ? dst_val >> shift_amount : dst_val << shift_amount;

    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 31),
        .z = res == 0,
        .v = BIT(res, 31) ^ BIT(dst_val, 31),
        .c = BIT(dst_val, do_shift_right ? shift_amount : 31 - shift_amount),
    };
    dst0.vtbl->write(&dst0, (uint16_t)res);
    if ((r_i & 1) == 0) dst1.vtbl->write(&dst1, (uint16_t)(res >> 16));
}

// multiple-percision

void pdp11_cpu_instr_adc(Pdp11Cpu *const self, Pdp11Word const dst) {
    uint16_t const res = dst.vtbl->read(&dst) + self->_psw.flags.c;
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 15),
        .z = res == 0,
        .v = self->_psw.flags.c && res == 0x8000,
        .c = self->_psw.flags.c && res == 0x0000,
    };
    dst.vtbl->write(&dst, res);
}
void pdp11_cpu_instr_adcb(Pdp11Cpu *const self, Pdp11Byte const dst) {
    uint8_t const res = dst.vtbl->read(&dst) + self->_psw.flags.c;
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 7),
        .z = res == 0,
        .v = self->_psw.flags.c && res == 0x80,
        .c = self->_psw.flags.c && res == 0x00,
    };
    dst.vtbl->write(&dst, res);
}
void pdp11_cpu_instr_sbc(Pdp11Cpu *const self, Pdp11Word const dst) {
    uint16_t const res = dst.vtbl->read(&dst) - self->_psw.flags.c;
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 15),
        .z = res == 0,
        .v = self->_psw.flags.c && res == 0x7FFF,
        .c = self->_psw.flags.c && res == 0xFFFF,
    };
    dst.vtbl->write(&dst, res);
}
void pdp11_cpu_instr_sbcb(Pdp11Cpu *const self, Pdp11Byte const dst) {
    uint8_t const res = dst.vtbl->read(&dst) - self->_psw.flags.c;
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 7),
        .z = res == 0,
        .v = self->_psw.flags.c && res == 0x7F,
        .c = self->_psw.flags.c && res == 0xFF,
    };
    dst.vtbl->write(&dst, res);
}

void pdp11_cpu_instr_sxt(Pdp11Cpu *const self, Pdp11Word const dst) {
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = self->_psw.flags.n,
        .z = !self->_psw.flags.n,
        .v = 0,
        .c = self->_psw.flags.c,
    };
    dst.vtbl->write(&dst, self->_psw.flags.n ? 0xFFFF : 0x0000);
}

// rotates

void pdp11_cpu_instr_ror(Pdp11Cpu *const self, Pdp11Word const dst) {
    uint16_t const dst_val = dst.vtbl->read(&dst);
    uint16_t const res = (dst_val >> 1) | (self->_psw.flags.c << 15);
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 15),
        .z = res == 0,
        .v = BIT(res, 15) ^ BIT(dst_val, 0),
        .c = BIT(dst_val, 0),
    };
    dst.vtbl->write(&dst, res);
}
void pdp11_cpu_instr_rorb(Pdp11Cpu *const self, Pdp11Byte const dst) {
    uint8_t const dst_val = dst.vtbl->read(&dst);
    uint8_t const res = (dst_val >> 1) | (self->_psw.flags.c << 7);
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 7),
        .z = res == 0,
        .v = BIT(res, 7) ^ BIT(dst_val, 0),
        .c = BIT(dst_val, 0),
    };
    dst.vtbl->write(&dst, res);
}
void pdp11_cpu_instr_rol(Pdp11Cpu *const self, Pdp11Word const dst) {
    uint16_t const dst_val = dst.vtbl->read(&dst);
    uint16_t const res = (dst_val << 1) | self->_psw.flags.c;
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 15),
        .z = res == 0,
        .v = BIT(res, 15) ^ BIT(dst_val, 15),
        .c = BIT(dst_val, 15),
    };
    dst.vtbl->write(&dst, res);
}
void pdp11_cpu_instr_rolb(Pdp11Cpu *const self, Pdp11Byte const dst) {
    uint8_t const dst_val = dst.vtbl->read(&dst);
    uint8_t const res = (dst_val << 1) | self->_psw.flags.c;
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 7),
        .z = res == 0,
        .v = BIT(res, 7) ^ BIT(dst_val, 7),
        .c = BIT(dst_val, 7),
    };
    dst.vtbl->write(&dst, res);
}

void pdp11_cpu_instr_swab(Pdp11Cpu *const self, Pdp11Word const dst) {
    uint16_t const dst_val = dst.vtbl->read(&dst);
    uint16_t const res =
        (uint16_t)(dst_val << 8) | (uint16_t)(uint8_t)(dst_val >> 8);
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 7),
        .z = (uint8_t)res == 0,
        .v = 0,
        .c = 0,
    };
    dst.vtbl->write(&dst, res);
}

// DUAL-OP

// arithmetic

void pdp11_cpu_instr_mov(
    Pdp11Cpu *const self,
    Pdp11Word const src,
    Pdp11Word const dst
) {
    uint16_t const src_val = src.vtbl->read(&src);
    pdp11_psw_set_flags_from_word(&self->_psw, src_val);
    dst.vtbl->write(&dst, src_val);
}
void pdp11_cpu_instr_movb(
    Pdp11Cpu *const self,
    Pdp11Byte const src,
    Pdp11Byte const dst
) {
    uint8_t const src_val = src.vtbl->read(&src);
    pdp11_psw_set_flags_from_byte(&self->_psw, src_val);
    if (dst.vtbl == &pdp11_cpu_reg_byte_vtbl) {
        Pdp11Word const dstw = pdp11_word_from_cpu_reg(self, dst.addr);
        pdp11_cpu_reg_word_write(&dstw, (int16_t)(int8_t)src_val);
    } else {
        dst.vtbl->write(&dst, src_val);
    }
}
void pdp11_cpu_instr_add(
    Pdp11Cpu *const self,
    Pdp11Word const src,
    Pdp11Word const dst
) {
    uint32_t const res =
        xword(src.vtbl->read(&src)) + xword(dst.vtbl->read(&dst));
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 15),
        .z = (uint16_t)res == 0,
        // TODO? maybe excessive and can be the same as in sub
        .v = BIT(res, 16) != BIT(res, 15),
        .c = BIT(res, 17),
    };
    dst.vtbl->write(&dst, res);
}
void pdp11_cpu_instr_sub(
    Pdp11Cpu *const self,
    Pdp11Word const src,
    Pdp11Word const dst
) {
    uint16_t const dst_val = dst.vtbl->read(&dst),
                   src_val = src.vtbl->read(&src);
    uint16_t const res = dst_val - src_val;
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 15),
        .z = (uint16_t)res == 0,
        .v = BIT(dst_val, 15) != BIT(src_val, 15) &&
             BIT(src_val, 15) == BIT(res, 15),
        .c = src_val > dst_val,
    };
    dst.vtbl->write(&dst, res);
}
void pdp11_cpu_instr_cmp(
    Pdp11Cpu *const self,
    Pdp11Word const src,
    Pdp11Word const dst
) {
    // NOTE args are swapped on purpose
    uint16_t const dst_val = src.vtbl->read(&src),
                   src_val = dst.vtbl->read(&dst);
    uint16_t const res = dst_val - src_val;
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 15),
        .z = (uint16_t)res == 0,
        .v = BIT(dst_val, 15) != BIT(src_val, 15) &&
             BIT(src_val, 15) == BIT(res, 15),
        .c = src_val > dst_val,
    };
}
void pdp11_cpu_instr_cmpb(
    Pdp11Cpu *const self,
    Pdp11Byte const src,
    Pdp11Byte const dst
) {
    // NOTE args are swapped on purpose
    uint8_t const dst_val = src.vtbl->read(&src),
                  src_val = dst.vtbl->read(&dst);
    uint8_t const res = dst_val - src_val;
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 7),
        .z = (uint8_t)res == 0,
        .v = BIT(dst_val, 7) != BIT(src_val, 7) &&
             BIT(src_val, 7) == BIT(res, 7),
        .c = src_val > dst_val,
    };
}

// register destination

void pdp11_cpu_instr_mul(
    Pdp11Cpu *const self,
    unsigned const r_i,
    Pdp11Word const src
) {
    Pdp11Word const dst0 = pdp11_word_from_cpu_reg(self, r_i),
                    dst1 = pdp11_word_from_cpu_reg(self, r_i | 1);

    uint32_t const res =
        (int16_t)dst0.vtbl->read(&dst0) * (int16_t)src.vtbl->read(&src);
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(res, 15),
        .z = (uint16_t)res == 0,
        .v = 0,
        .c = (uint16_t)(res >> 16) != (BIT(res, 15) ? 0xFFFF : 0x0000),
    };
    dst0.vtbl->write(&dst0, (uint16_t)res);
    if ((r_i & 1) == 0) dst1.vtbl->write(&dst1, (uint16_t)(res >> 16));
}
void pdp11_cpu_instr_div(
    Pdp11Cpu *const self,
    unsigned const r_i,
    Pdp11Word const src
) {
    Pdp11Word const dst0 = pdp11_word_from_cpu_reg(self, r_i),
                    dst1 = pdp11_word_from_cpu_reg(self, r_i | 1);

    uint16_t const src_val = src.vtbl->read(&src);
    if (src_val == 0) {
        self->_psw.flags.v = self->_psw.flags.c = 1;
        return;
    }
    uint32_t const dst_val =
        dst0.vtbl->read(&dst0) |
        ((r_i & 1) == 0 ? dst1.vtbl->read(&dst1) << 16 : 0);

    uint32_t const quotient = dst_val / src_val;
    uint32_t const remainder = dst_val % src_val;
    if ((uint16_t)(quotient >> 16) != 0) {
        self->_psw.flags.v = 1;
        return;
    }
    self->_psw.flags = (Pdp11PswFlags){
        .t = self->_psw.flags.t,
        .n = BIT(quotient, 15),
        .z = quotient == 0,
        .v = 0,
        .c = 0,
    };
    dst0.vtbl->write(&dst0, quotient);
    if ((r_i & 1) == 0) dst1.vtbl->write(&dst1, remainder);
}

void pdp11_cpu_instr_xor(
    Pdp11Cpu *const self,
    unsigned const r_i,
    Pdp11Word const dst
) {
    Pdp11Word const src = pdp11_word_from_cpu_reg(self, r_i);
    uint16_t const res = dst.vtbl->read(&dst) ^ src.vtbl->read(&src);
    pdp11_psw_set_flags_from_word(&self->_psw, res);
    dst.vtbl->write(&dst, res);
}

// logical

void pdp11_cpu_instr_bit(
    Pdp11Cpu *const self,
    Pdp11Word const src,
    Pdp11Word const dst
) {
    pdp11_psw_set_flags_from_word(
        &self->_psw,
        dst.vtbl->read(&dst) & src.vtbl->read(&src)
    );
}
void pdp11_cpu_instr_bitb(
    Pdp11Cpu *const self,
    Pdp11Byte const src,
    Pdp11Byte const dst
) {
    pdp11_psw_set_flags_from_byte(
        &self->_psw,
        dst.vtbl->read(&dst) & src.vtbl->read(&src)
    );
}
void pdp11_cpu_instr_bis(
    Pdp11Cpu *const self,
    Pdp11Word const src,
    Pdp11Word const dst
) {
    dst.vtbl->write(&dst, dst.vtbl->read(&dst) | src.vtbl->read(&src));
    pdp11_cpu_instr_bit(self, dst, dst);
}
void pdp11_cpu_instr_bisb(
    Pdp11Cpu *const self,
    Pdp11Byte const src,
    Pdp11Byte const dst
) {
    dst.vtbl->write(&dst, dst.vtbl->read(&dst) | src.vtbl->read(&src));
    pdp11_cpu_instr_bitb(self, dst, dst);
}
void pdp11_cpu_instr_bic(
    Pdp11Cpu *const self,
    Pdp11Word const src,
    Pdp11Word const dst
) {
    dst.vtbl->write(&dst, dst.vtbl->read(&dst) & ~src.vtbl->read(&src));
    pdp11_cpu_instr_bit(self, dst, dst);
}
void pdp11_cpu_instr_bicb(
    Pdp11Cpu *const self,
    Pdp11Byte const src,
    Pdp11Byte const dst
) {
    dst.vtbl->write(&dst, dst.vtbl->read(&dst) & ~src.vtbl->read(&src));
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
    if (self->_psw.flags.z == cond) pdp11_cpu_instr_br(self, off);
}
void pdp11_cpu_instr_bpl_bmi(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
) {
    if (self->_psw.flags.n == cond) pdp11_cpu_instr_br(self, off);
}
void pdp11_cpu_instr_bcc_bcs(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
) {
    if (self->_psw.flags.c == cond) pdp11_cpu_instr_br(self, off);
}
void pdp11_cpu_instr_bvc_bvs(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
) {
    if (self->_psw.flags.v == cond) pdp11_cpu_instr_br(self, off);
}

void pdp11_cpu_instr_bge_bl(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
) {
    if ((self->_psw.flags.n ^ self->_psw.flags.v) == cond)
        pdp11_cpu_instr_br(self, off);
}
void pdp11_cpu_instr_bg_ble(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
) {
    if ((self->_psw.flags.z || self->_psw.flags.n ^ self->_psw.flags.v) == cond)
        pdp11_cpu_instr_br(self, off);
}

void pdp11_cpu_instr_bhi_blos(
    Pdp11Cpu *const self,
    bool const cond,
    uint8_t const off
) {
    if ((self->_psw.flags.c || self->_psw.flags.z) == cond)
        pdp11_cpu_instr_br(self, off);
}

// subroutine

void pdp11_cpu_instr_jsr_jmp(
    Pdp11Cpu *const self,
    unsigned const r_i,
    unsigned const mode
) {
    uint16_t const effective_addr =
        pdp11_cpu_jmp_jsr_effective_addr(self, mode);
    if (r_i < PDP11_CPU_REG_COUNT) {
        Pdp11Word const dst = pdp11_word_from_cpu_reg(self, r_i);
        if (pdp11_stack_push(self, dst.vtbl->read(&dst)) != Ok)
            return pdp11_cpu_trap(self, PDP11_CPU_TRAP_CPU_ERR);
        dst.vtbl->write(&dst, pdp11_cpu_pc(self));
    }
    pdp11_cpu_pc(self) = effective_addr;
}
void pdp11_cpu_instr_mark(Pdp11Cpu *const self, unsigned const param_count) {
    pdp11_cpu_sp(self) = pdp11_cpu_pc(self) + 2 * param_count;
    pdp11_cpu_pc(self) = pdp11_cpu_rx(self, 5);
    if (pdp11_stack_pop(self, &pdp11_cpu_rx(self, 5)) != Ok)
        return pdp11_cpu_trap(self, PDP11_CPU_TRAP_CPU_ERR);
}
void pdp11_cpu_instr_rts(Pdp11Cpu *const self, unsigned const r_i) {
    pdp11_cpu_pc(self) = pdp11_cpu_rx(self, r_i);
    if (pdp11_stack_pop(self, &pdp11_cpu_rx(self, r_i)) != Ok)
        return pdp11_cpu_trap(self, PDP11_CPU_TRAP_CPU_ERR);
}

// program control

void pdp11_cpu_instr_spl(Pdp11Cpu *const self, uint8_t const value) {
    pdp11_psw_set_priority(&self->_psw, value);
}

void pdp11_cpu_instr_sob(
    Pdp11Cpu *const self,
    unsigned const r_i,
    uint8_t const off
) {
    Pdp11Word const dst = pdp11_word_from_cpu_reg(self, r_i);
    uint16_t const res = dst.vtbl->read(&dst) - 1;
    dst.vtbl->write(&dst, res);
    if (res != 0) pdp11_cpu_pc(self) -= 2 * off;
}

// trap

void pdp11_cpu_instr_emt(Pdp11Cpu *const self) {
    pdp11_cpu_trap(self, PDP11_CPU_TRAP_EMT);
}
void pdp11_cpu_instr_trap(Pdp11Cpu *const self) {
    pdp11_cpu_trap(self, PDP11_CPU_TRAP_TRAP);
}
void pdp11_cpu_instr_bpt(Pdp11Cpu *const self) {
    pdp11_cpu_trap(self, PDP11_CPU_TRAP_BPT);
}
void pdp11_cpu_instr_iot(Pdp11Cpu *const self) {
    pdp11_cpu_trap(self, PDP11_CPU_TRAP_IOT);
}
void pdp11_cpu_instr_rti_rtt(Pdp11Cpu *const self) {
    uint16_t psw_word;
    if (pdp11_stack_pop(self, &pdp11_cpu_pc(self)) != Ok ||
        pdp11_stack_pop(self, &psw_word) != Ok)
        return pdp11_cpu_trap(self, PDP11_CPU_TRAP_CPU_ERR);
    pdp11_psw_set(&self->_psw, psw_word);
}

// MISC.

void pdp11_cpu_instr_halt(Pdp11Cpu *const self) {
    self->_state = PDP11_CPU_STATE_HALT;
}
void pdp11_cpu_instr_wait(Pdp11Cpu *const self) {
    self->_state = PDP11_CPU_STATE_WAIT;
}
void pdp11_cpu_instr_reset(Pdp11Cpu *const self) {
    unibus_reset(self->_unibus);
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
    if (do_affect_nf) self->_psw.flags.n = value;
    if (do_affect_zf) self->_psw.flags.z = value;
    if (do_affect_vf) self->_psw.flags.v = value;
    if (do_affect_cf) self->_psw.flags.c = value;
}
