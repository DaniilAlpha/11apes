#ifndef PDP11_CPU_H
#define PDP11_CPU_H

#include <stdbool.h>
#include <stdint.h>

#include "pdp11/cpu/pdp11_cpu_stat.h"
#include "pdp11/pdp11_ram.h"

#define PDP11_CPU_REG_COUNT (8)

typedef struct Pdp11Cpu {
    Pdp11CpuStat stat;
    uint16_t _r[PDP11_CPU_REG_COUNT];

    Pdp11Ram *_ram;
} Pdp11Cpu;

void pdp11_cpu_init(
    Pdp11Cpu *const self,
    Pdp11Ram *const ram,
    uint16_t const pc,
    Pdp11CpuStat const stat
);
void pdp11_cpu_uninit(Pdp11Cpu *const self);

#define pdp11_cpu_rx(SELF_, I_) (*(uint16_t *)((SELF_)->_r + (I_)))
#define pdp11_cpu_rl(SELF_, I_) (*(uint8_t *)((SELF_)->_r + (I_)))
#define pdp11_cpu_pc(SELF_)     pdp11_cpu_rx((SELF_), 7)
#define pdp11_cpu_sp(SELF_)     pdp11_cpu_rx((SELF_), 6)

void pdp11_cpu_exec_instr(Pdp11Cpu *const self, uint16_t const instr);

#endif
