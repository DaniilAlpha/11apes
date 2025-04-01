#ifndef PDP11_CPU_STAT_H
#define PDP11_CPU_STAT_H

#include <stdbool.h>
#include <stdint.h>

#include "bits.h"

typedef struct Pdp11CpuStat {
    uint8_t priority : 3;
    bool tf : 1, nf : 1, zf : 1, vf : 1, cf : 1;
} Pdp11CpuStat;

static inline uint16_t pdp11_cpu_stat_to_word(Pdp11CpuStat *const self) {
    return self->priority << 5 | self->tf << 4 | self->nf << 3 | self->zf << 2 |
           self->vf << 1 | self->cf << 0;
}
static inline Pdp11CpuStat pdp11_cpu_stat(uint16_t const word) {
    return (Pdp11CpuStat){
        .priority = BITS(word, 5, 7),
        .tf = BIT(word, 4),
        .nf = BIT(word, 3),
        .zf = BIT(word, 2),
        .vf = BIT(word, 1),
        .cf = BIT(word, 0),
    };
}

#endif
