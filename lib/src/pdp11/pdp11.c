#include "pdp11/pdp11.h"

#include <stdio.h>

Result pdp11_init(Pdp11 *const self) {
    UNROLL(pdp11_ram_init(&self->ram));
    pdp11_cpu_init(
        &self->cpu,
        &self->ram,
        PDP11_STARTUP_PC,
        PDP11_STARTUP_CPU_STAT
    );

    return Ok;
}
void pdp11_uninit(Pdp11 *const self) {
    pdp11_ram_uninit(&self->ram);
    pdp11_cpu_uninit(&self->cpu);
}

uint16_t pdp11_instr_next(Pdp11 *const self) {
    uint16_t const instr = pdp11_ram_word(&self->ram, pdp11_cpu_pc(&self->cpu));
    pdp11_cpu_pc(&self->cpu) += 2;
    return instr;
}

void pdp11_step(Pdp11 *const self) {
    uint16_t const instr = pdp11_instr_next(self);

    printf(
        "exec: 0%06o\tps: p%03o %s %s%s%s%s -> ",
        instr,
        self->cpu.stat.priority,
        self->cpu.stat.tf ? "T" : ".",
        self->cpu.stat.nf ? "N" : ".",
        self->cpu.stat.zf ? "Z" : ".",
        self->cpu.stat.vf ? "V" : ".",
        self->cpu.stat.cf ? "C" : "."
    );
    pdp11_cpu_exec_instr(&self->cpu, instr);
    printf(
        "p%03o %s %s%s%s%s\n",
        self->cpu.stat.priority,
        self->cpu.stat.tf ? "T" : ".",
        self->cpu.stat.nf ? "N" : ".",
        self->cpu.stat.zf ? "Z" : ".",
        self->cpu.stat.vf ? "V" : ".",
        self->cpu.stat.cf ? "C" : "."
    );
}
