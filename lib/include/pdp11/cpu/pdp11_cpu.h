#ifndef PDP11_CPU_H
#define PDP11_CPU_H

#include <stdbool.h>
#include <stdint.h>

#include <semaphore.h>

#include "pdp11/cpu/pdp11_cpu_instr.h"
#include "pdp11/cpu/pdp11_cpu_stat.h"
#include "pdp11/unibus/unibus.h"

#define PDP11_CPU_REG_COUNT (8)

enum {
    PDP11_CPU_NO_TRAP = 0000,  // NOTE assumes 'zero' as no trap

    PDP11_CPU_TRAP_CPU_ERR = 0004,
    PDP11_CPU_TRAP_CPU_STACK_OVERFLOW =
        0004,  // TODO? maybe detect stack overflow

    PDP11_CPU_TRAP_ILLEGAL_INSTR = 0010,

    PDP11_CPU_TRAP_BPT = 0014,
    PDP11_CPU_TRAP_IOT = 0020,

    PDP11_CPU_TRAP_POWER_FAIL = 0024,

    PDP11_CPU_TRAP_EMT = 0030,
    PDP11_CPU_TRAP_TRAP = 0034,
};

typedef struct Pdp11Cpu {
    Pdp11CpuStat _stat;
    uint16_t _r[PDP11_CPU_REG_COUNT];

    _Atomic uint8_t __pending_intr;
    sem_t __pending_intr_sem;

    Unibus *_unibus;
    enum {
        PDP11_CPU_STATE_RUNNING,
        PDP11_CPU_STATE_HALTED,
        PDP11_CPU_STATE_WAITING
    } volatile _state;  // TODO? maybe redo with condition vars
} Pdp11Cpu;

void pdp11_cpu_init(Pdp11Cpu *const self, Unibus *const unibus);
void pdp11_cpu_uninit(Pdp11Cpu *const self);
void pdp11_cpu_reset(Pdp11Cpu *const self);

static inline uint16_t *pdp11_cpu_rx(Pdp11Cpu *const self, unsigned const i) {
    return (uint16_t *)(self->_r + i);
}
#define pdp11_cpu_rx(SELF_, I_) (*pdp11_cpu_rx((SELF_), (I_)))
static inline uint8_t *pdp11_cpu_rl(Pdp11Cpu *const self, unsigned const i) {
    return (uint8_t *)(self->_r + i);
}
#define pdp11_cpu_rl(SELF_, I_) (*pdp11_cpu_rl((SELF_), (I_)))
#define pdp11_cpu_pc(SELF_)     pdp11_cpu_rx((SELF_), 7)
#define pdp11_cpu_sp(SELF_)     pdp11_cpu_rx((SELF_), 6)

static inline Pdp11CpuStat *pdp11_cpu_stat(Pdp11Cpu *const self) {
    return &self->_stat;
}
#define pdp11_cpu_stat(SELF_) (*pdp11_cpu_stat(SELF_))

void pdp11_cpu_intr(Pdp11Cpu *const self, uint8_t const intr);
void pdp11_cpu_halt(Pdp11Cpu *const self);
void pdp11_cpu_continue(Pdp11Cpu *const self);

uint16_t pdp11_cpu_fetch(Pdp11Cpu *const self);
Pdp11CpuInstr pdp11_cpu_decode(Pdp11Cpu *const self, uint16_t const encoded);
void pdp11_cpu_exec(Pdp11Cpu *const self, Pdp11CpuInstr const instr);

#endif
