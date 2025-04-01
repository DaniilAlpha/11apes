#ifndef PDP11_RAM_H
#define PDP11_RAM_H

#include <stdint.h>

#include <result.h>

// PDP-11 can address has 32K words of RAM, but top 4096 are reserved
#define PDP11_RAM_WORD_COUNT     (32 * 1024 - 4096)
#define PDP11_ARRD_STACK_BOTTOM  (0400)
#define PDP11_ARRD_PERIPH_BOTTOM (160000)

typedef struct Pdp11Ram {
    void *_ram;
} Pdp11Ram;

Result pdp11_ram_init(Pdp11Ram *const self);
void pdp11_ram_uninit(Pdp11Ram *const self);

static inline uint16_t *
internal__pdp11_ram_word(Pdp11Ram *const self, uint16_t const addr) {
    return (uint16_t *)(self->_ram + addr);
}
#define pdp11_ram_word(SELF_, ADDR_)                                           \
    (*internal__pdp11_ram_word((SELF_), (ADDR_)))

static inline uint8_t *
internal__pdp11_ram_byte(Pdp11Ram *const self, uint16_t const addr) {
    return (uint8_t *)(self->_ram + addr);
}
#define pdp11_ram_byte(SELF_, ADDR_)                                           \
    (*internal__pdp11_ram_byte((SELF_), (ADDR_)))

#endif
