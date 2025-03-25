#include "pdp11/pdp11.h"

#include <stdio.h>
#include <stdlib.h>

#include "conviniences.h"
#include "pdp11/pdp11_op.h"

Result pdp11_init(Pdp11 *const self) {
    // ram
    void *const ram = malloc(PDP11_RAM_WORD_COUNT * sizeof(uint16_t));
    if (!ram) return OutOfMemErr;
    self->_ram = ram;

    // cpu
    self->cpu.r[7] = PDP11_STARTUP_PC;
    self->cpu.ps = PDP11_STARTUP_PS;

    return Ok;
}
void pdp11_uninit(Pdp11 *const self) {
    // ram
    free(self->_ram), self->_ram = NULL;

    // cpu
    self->cpu.r[7] = 0;
    self->cpu.ps = (Pdp11Ps){0};
}

uint16_t pdp11_instr_next(Pdp11 *const self) {
    uint16_t const instr = pdp11_ram_word_at(self, self->cpu.r[7]);
    self->cpu.r[7] += 2;
    return instr;
}

void pdp11_step(Pdp11 *const self) {
    pdp11_op_exec(self, pdp11_instr_next(self));
}
