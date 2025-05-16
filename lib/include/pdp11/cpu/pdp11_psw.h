#ifndef PDP11_PSW_H
#define PDP11_PSW_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include <bits/pthreadtypes.h>
#include <result.h>

#include "bits.h"

typedef struct Pdp11PswFlags {
    bool t : 1, n : 1, z : 1, v : 1, c : 1;
} Pdp11PswFlags;
typedef struct Pdp11Psw {
    uint8_t volatile _priority : 3;
    pthread_cond_t _priority_changed;
    Pdp11PswFlags volatile flags;
} Pdp11Psw;

Result pdp11_psw_init(Pdp11Psw *const self);
void pdp11_psw_uninit(Pdp11Psw *const self);

static inline uint8_t pdp11_psw_priority(Pdp11Psw const *const self) {
    return self->_priority;
}
static inline void
pdp11_psw_set_priority(Pdp11Psw *const self, uint8_t const value) {
    self->_priority = value;
    pthread_cond_signal(&self->_priority_changed);
}

static inline uint16_t pdp11_psw_to_word(Pdp11Psw const *const self) {
    return self->_priority << 5 | self->flags.t << 4 | self->flags.n << 3 |
           self->flags.z << 2 | self->flags.v << 1 | self->flags.c << 0;
}
static inline void pdp11_psw_set(Pdp11Psw *const self, uint16_t const value) {
    self->flags = (Pdp11PswFlags){
        .t = BIT(value, 4),
        .n = BIT(value, 3),
        .z = BIT(value, 2),
        .v = BIT(value, 1),
        .c = BIT(value, 0),
    };
    pdp11_psw_set_priority(self, BITS(value, 5, 7));
}

void pdp11_psw_wait_for_sufficient_priority(
    Pdp11Psw *const self,
    uint8_t const priority,
    pthread_mutex_t *const lock
);

#endif
