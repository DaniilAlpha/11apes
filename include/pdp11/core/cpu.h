#ifndef PDP11_CORE_CPU_H
#define PDP11_CORE_CPU_H

#include <stdint.h>

#define CPU_REGISTER_COUNT (8)

typedef struct Cpu {
    union {
        uint16_t r[CPU_REGISTER_COUNT];
        /* struct {
            uint16_t __[CPU_REGISTER_COUNT - 2];
            uint16_t sp, pc;
        } special; */
    } registers;
    uint16_t psw;
} Cpu;

void cpu_init(
    Cpu *const self,
    uint16_t const initial_pc,
    uint16_t const initial_psw
);

#endif
