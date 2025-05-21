#ifndef BITS_H
#define BITS_H

#include <assert.h>

#define INTERNAL__BITS(NUM_, START_I_, END_I_)                                 \
    (((NUM_) >> (START_I_)) & ((1 << ((END_I_) + 1 - (START_I_))) - 1))
static_assert(INTERNAL__BITS(0x1120, 0, 3) == 0);
static_assert(INTERNAL__BITS(0x1120, 4, 7) == 2);
static_assert(INTERNAL__BITS(0x1120, 0, 7) == 0x20);
static_assert(INTERNAL__BITS(0x1120, 12, 15) == 1);

static inline unsigned long
bitsl(unsigned long const num, unsigned const start_i, unsigned const end_i) {
    return INTERNAL__BITS(num, start_i, end_i);
}
static inline unsigned int
bits(unsigned int const num, unsigned const start_i, unsigned const end_i) {
    return INTERNAL__BITS(num, start_i, end_i);
}
static inline unsigned short
bitsh(unsigned short const num, unsigned const start_i, unsigned const end_i) {
    return INTERNAL__BITS(num, start_i, end_i);
}
static inline unsigned char
bitsb(unsigned char const num, unsigned const start_i, unsigned const end_i) {
    return INTERNAL__BITS(num, start_i, end_i);
}
#undef INTERNAL__BITS

#define BIT(NUM_, I_) (((NUM_) >> (I_)) & 1)
#define BITS(NUM_, START_I_, END_I_)                                           \
    _Generic(                                                                  \
        (NUM_),                                                                \
                                                                               \
        unsigned char: bitsb,                                                  \
        signed char: bitsb,                                                    \
                                                                               \
        unsigned short: bitsh,                                                 \
        signed short: bitsh,                                                   \
                                                                               \
        unsigned: bits,                                                        \
        signed: bits,                                                          \
                                                                               \
        unsigned long: bitsl,                                                  \
        signed long: bitsl                                                     \
                                                                               \
    )((NUM_), (START_I_), (END_I_))

#endif
