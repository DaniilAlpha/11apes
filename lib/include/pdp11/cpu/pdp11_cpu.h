#ifndef PDP11_CPU_H
#define PDP11_CPU_H

#include <stdbool.h>
#include <stdint.h>

#include "pdp11/cpu/pdp11_cpu_stat.h"
#include "pdp11/unibus/unibus.h"

#define PDP11_CPU_REG_COUNT (8)

typedef struct Pdp11Cpu {
    Pdp11CpuStat stat;
    uint16_t __r[PDP11_CPU_REG_COUNT];

    Unibus *_unibus;
} Pdp11Cpu;

typedef enum Pdp11CpuTrap {
    PDP11_CPU_TRAP_CPU_ERR = 0004,
    PDP11_CPU_TRAP_CPU_STACK_OVERFLOW = 0004,
    PDP11_CPU_TRAP_ILLEGAL_INSTR = 0010,
    PDP11_CPU_TRAP_BPT = 0014,
    PDP11_CPU_TRAP_IOT = 0020,
    PDP11_CPU_TRAP_POWER_FAIL = 0024,
    PDP11_CPU_TRAP_EMT = 0030,
    PDP11_CPU_TRAP_TRAP = 0034,
    PDP11_CPU_TRAP_MEM_ERR = 0114,
    PDP11_CPU_TRAP_PROGRAM_IRQ = 0240,
    PDP11_CPU_TRAP_FP_ERR = 0240,
} Pdp11CpuTrap;

void pdp11_cpu_init(
    Pdp11Cpu *const self,
    Unibus *const unibus,
    uint16_t const pc,
    Pdp11CpuStat const stat
);
void pdp11_cpu_uninit(Pdp11Cpu *const self);

// TODO `volatile` here are for what happens if someone interrupts CPU, but it
// decides to cache the PC. interrupt may not happen in that case, which is
// really undesireble. This can only be made volatile together

static inline uint16_t volatile *
pdp11_cpu_rx(Pdp11Cpu *const self, unsigned const i) {
    return (uint16_t *)(self->__r + i);
}
#define pdp11_cpu_rx(SELF_, I_) (*pdp11_cpu_rx((SELF_), (I_)))
static inline uint8_t volatile *
pdp11_cpu_rl(Pdp11Cpu *const self, unsigned const i) {
    return (uint8_t *)(self->__r + i);
}
#define pdp11_cpu_rl(SELF_, I_) (*pdp11_cpu_rl((SELF_), (I_)))
#define pdp11_cpu_pc(SELF_)     pdp11_cpu_rx((SELF_), 7)
#define pdp11_cpu_sp(SELF_)     pdp11_cpu_rx((SELF_), 6)

void pdp11_cpu_trap(Pdp11Cpu *const self, Pdp11CpuTrap const trap);

uint16_t pdp11_cpu_fetch(Pdp11Cpu *const self);
void pdp11_cpu_decode_exec(Pdp11Cpu *const self, uint16_t const instr);

#endif
