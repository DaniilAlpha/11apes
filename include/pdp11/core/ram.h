#ifndef PDP11_CORE_RAM_H
#define PDP11_CORE_RAM_H

#include <stdint.h>

#include <result.h>

// TODO!!! account for wierd endianness
/*
16-bit words are stored little-endian with least significant bytes at the
lower address. Words are always aligned to even memory addresses. Words can be
held in registers R0 through R7.

32-bit double words in the Extended Instruction Set (EIS) can only be stored
in register pairs with the lower word being stored in the lower-numbered
register. Double words are used by the MUL, DIV, and ASHC instructions. Other
32-bit data are supported as extensions to the basic architecture: floating
point in the FPU Instruction Set or long data in the Commercial Instruction
Set are stored in more than one format, including an unusual middle-endian
format[https://en.wikipedia.org/wiki/Endianness#Middle-endian] sometimes
referred to as "PDP-endian."*/

#define RAM_SIZE (56 * 1024)

typedef struct Ram {
    uint8_t *byte;
} Ram;

Result ram_init(Ram *const self);
void ram_uninit(Ram *const self);

#endif
