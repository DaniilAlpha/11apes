#ifndef CONVINIENCES_H
#define CONVINIENCES_H

#define elsizeof(ARR_) (sizeof(*ARR_))
#define lenof(ARR_)    (sizeof(ARR_) / elsizeof(ARR_))

#define forceinline __attribute__((always_inline)) inline

#define bits(NUM_, START_I_, END_I_)                                           \
    (((NUM_) >> (START_I_)) & ((1 << ((END_I_) + 1 - (START_I_))) - 1))
#define bit(NUM_, I_) bits((NUM_), (I_), (I_) + 1)

#endif
