#include "pdp11/pdp11.h"

#include <stdio.h>
#include <stdlib.h>

#include "pdp11/pdp11_op.h"

Result pdp11_init(Pdp11 *const self) {
    UNROLL(pdp11_ram_init(&self->_ram));
    pdp11_cpu_init(
        &self->_cpu,
        &self->_ram,
        PDP11_STARTUP_PC,
        PDP11_STARTUP_CPU_STAT
    );

    return Ok;
}
void pdp11_uninit(Pdp11 *const self) {
    pdp11_ram_uninit(&self->_ram);
    pdp11_cpu_uninit(&self->_cpu);
}

uint16_t pdp11_instr_next(Pdp11 *const self) {
    uint16_t const instr =
        *pdp11_ram_word(&self->_ram, pdp11_cpu_pc(&self->_cpu));
    pdp11_cpu_pc(&self->_cpu) += 2;
    return instr;
}

void pdp11_step(Pdp11 *const self) {
    uint16_t const instr = pdp11_instr_next(self);

    printf(
        "exec: 0%06o\tps: p%03o %s %s%s%s%s -> ",
        instr,
        self->_cpu._stat.priority,
        self->_cpu._stat.tf ? "T" : ".",
        self->_cpu._stat.nf ? "N" : ".",
        self->_cpu._stat.zf ? "Z" : ".",
        self->_cpu._stat.vf ? "V" : ".",
        self->_cpu._stat.cf ? "C" : "."
    );
    pdp11_op_exec(self, instr);
    printf(
        "p%03o %s %s%s%s%s\n",
        self->_cpu._stat.priority,
        self->_cpu._stat.tf ? "T" : ".",
        self->_cpu._stat.nf ? "N" : ".",
        self->_cpu._stat.zf ? "Z" : ".",
        self->_cpu._stat.vf ? "V" : ".",
        self->_cpu._stat.cf ? "C" : "."
    );
}

void pdp11_stack_push(Pdp11 *const self, uint16_t const value) {
    *pdp11_ram_word(&self->_ram, pdp11_cpu_sp(&self->_cpu)) = value;
    pdp11_cpu_sp(&self->_cpu) -= 2;
}
uint16_t pdp11_stack_pop(Pdp11 *const self) {
    pdp11_cpu_sp(&self->_cpu) += 2;
    return *pdp11_ram_word(&self->_ram, pdp11_cpu_sp(&self->_cpu));
}
