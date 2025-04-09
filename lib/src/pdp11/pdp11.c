#include "pdp11/pdp11.h"

#include <pthread.h>
#include <stdio.h>

#include "pdp11/unibus/mutex_unibus_lock.h"

/*************
 ** private **
 *************/

static void pdp11_cpu_thread_helper(Pdp11 *const self) {
    while (!self->_should_stop) {
        uint16_t const instr = pdp11_cpu_fetch(&self->cpu);

        printf(
            "ps = %1o%s%s%s%s%s \t exec: 0%06o \t ",
            self->cpu.stat.priority,
            self->cpu.stat.tf ? "T" : "t",
            self->cpu.stat.nf ? "N" : "n",
            self->cpu.stat.zf ? "Z" : "z",
            self->cpu.stat.vf ? "V" : "v",
            self->cpu.stat.cf ? "C" : "c",
            instr
        );
        pdp11_cpu_decode_exec(&self->cpu, instr);
        printf(
            "ps = %1o%s%s%s%s%s\n",
            self->cpu.stat.priority,
            self->cpu.stat.tf ? "T" : "t",
            self->cpu.stat.nf ? "N" : "n",
            self->cpu.stat.zf ? "Z" : "z",
            self->cpu.stat.vf ? "V" : "v",
            self->cpu.stat.cf ? "C" : "c"
        );
    }
}
static void *pdp11_cpu_thread(void *const vself) {
    return pdp11_cpu_thread_helper(vself), NULL;
};

/************
 ** public **
 ************/

Result pdp11_init(Pdp11 *const self) {
    UNROLL(pdp11_ram_init(&self->ram));

    if (pthread_mutex_init(&self->_sack_lock, NULL) != 0 ||
        pthread_mutex_init(&self->_sack_lock, NULL) != 0)
        return pdp11_ram_uninit(&self->ram), RangeErr;

    unibus_init(
        &self->unibus,
        &self->cpu,
        pthread_mutex_ww_unibus_lock(&self->_sack_lock),
        pthread_mutex_ww_unibus_lock(&self->_bbsy_lock)
    );

    pdp11_cpu_init(
        &self->cpu,
        &self->unibus,
        PDP11_STARTUP_PC,
        PDP11_STARTUP_CPU_STAT
    );

    self->_should_stop = true;

    return Ok;
}
void pdp11_uninit(Pdp11 *const self) {
    if (!self->_should_stop) pdp11_stop(self);

    pdp11_cpu_uninit(&self->cpu);
    pthread_mutex_destroy(&self->_bbsy_lock);
    pthread_mutex_destroy(&self->_sack_lock);
    pdp11_ram_uninit(&self->ram);
}

void pdp11_start(Pdp11 *const self) {
    self->_should_stop = false;
    pthread_create(&self->_cpu_thread, NULL, pdp11_cpu_thread, self);
}
void pdp11_stop(Pdp11 *const self) {
    self->_should_stop = true;
    pthread_join(self->_cpu_thread, NULL);
}
