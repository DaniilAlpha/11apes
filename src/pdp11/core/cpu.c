#include "pdp11/core/cpu.h"

void cpu_init(
    Cpu *const self,
    uint16_t const initial_pc,
    uint16_t const initial_psw
) {
    self->registers.r[7] = initial_pc;
    self->psw = initial_psw;
}
