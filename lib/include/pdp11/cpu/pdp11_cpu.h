#ifndef PDP11_CPU_H
#define PDP11_CPU_H

#include <stdbool.h>
#include <stdint.h>

#include <semaphore.h>

#include "pdp11/cpu/pdp11_psw.h"
#include "pdp11/unibus/unibus.h"

#define PDP11_CPU_REG_COUNT (8)

enum {
    PDP11_CPU_NO_TRAP = 0000,  // NOTE assumes 'zero' as no trap

    PDP11_CPU_TRAP_CPU_ERR = 0004,

    PDP11_CPU_TRAP_ILLEGAL_INSTR = 0010,

    PDP11_CPU_TRAP_BPT = 0014,
    PDP11_CPU_TRAP_IOT = 0020,

    PDP11_CPU_TRAP_EMT = 0030,
    PDP11_CPU_TRAP_TRAP = 0034,
};

typedef enum Pdp11CpuState {
    PDP11_CPU_STATE_RUN,
    PDP11_CPU_STATE_HALT,
    PDP11_CPU_STATE_WAIT,
    PDP11_CPU_STATE_STEP,
} Pdp11CpuState;

typedef struct Pdp11Cpu {
    Pdp11Psw _psw;
    uint16_t _r[PDP11_CPU_REG_COUNT];

    _Atomic uint8_t __pending_intr;
    sem_t __pending_intr_sem;

    Unibus *_unibus;
    Pdp11CpuState volatile _state;  // TODO? maybe redo with condition vars

    pthread_t _thread;
} Pdp11Cpu;

void pdp11_cpu_init(Pdp11Cpu *const self, Unibus *const unibus);
void pdp11_cpu_uninit(Pdp11Cpu *const self);
void pdp11_cpu_reset(Pdp11Cpu *const self);

static inline uint16_t volatile *
pdp11_cpu_rx(Pdp11Cpu *const self, unsigned const i) {
    return (uint16_t *)(self->_r + i);
}
#define pdp11_cpu_rx(SELF_, I_) (*pdp11_cpu_rx((SELF_), (I_)))
#define pdp11_cpu_pc(SELF_)     pdp11_cpu_rx((SELF_), 7)

static inline Pdp11Psw volatile *pdp11_cpu_psw(Pdp11Cpu *const self) {
    return &self->_psw;
}
#define pdp11_cpu_psw(SELF_) (*pdp11_cpu_psw(SELF_))

static inline Pdp11CpuState pdp11_cpu_state(Pdp11Cpu const *const self) {
    return self->_state;
}

void pdp11_cpu_intr(Pdp11Cpu *const self, uint8_t const intr);

void pdp11_cpu_halt(Pdp11Cpu *const self);
void pdp11_cpu_continue(Pdp11Cpu *const self);
void pdp11_cpu_single_step(Pdp11Cpu *const self);

#endif
