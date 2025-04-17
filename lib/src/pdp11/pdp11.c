#include "pdp11/pdp11.h"

#include <pthread.h>
#include <stdio.h>

#include <unistd.h>

#include "pdp11/unibus/mutex_unibus_lock.h"

/*************
 ** private **
 *************/

static void pdp11_cpu_thread_helper(Pdp11 *const self) {
    while (self->_should_run) {
        usleep(100 * 1000);
        uint16_t const instr = pdp11_cpu_fetch(&self->cpu);

        printf(
            "pc = 0%06o \t ps = %1o%s%s%s%s%s \t exec: 0%06o \t ",
            pdp11_cpu_pc(&self->cpu),
            self->cpu._stat.priority,
            self->cpu._stat.tf ? "T" : "t",
            self->cpu._stat.nf ? "N" : "n",
            self->cpu._stat.zf ? "Z" : "z",
            self->cpu._stat.vf ? "V" : "v",
            self->cpu._stat.cf ? "C" : "c",
            instr
        );
        pdp11_cpu_decode_exec(&self->cpu, instr);
        printf(
            "ps = %1o%s%s%s%s%s\n",
            self->cpu._stat.priority,
            self->cpu._stat.tf ? "T" : "t",
            self->cpu._stat.nf ? "N" : "n",
            self->cpu._stat.zf ? "Z" : "z",
            self->cpu._stat.vf ? "V" : "v",
            self->cpu._stat.cf ? "C" : "c"
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
    UNROLL(pdp11_ram_init(&self->ram, 0, 16 * 1024 * 2, false));

    if (pthread_mutex_init(&self->_sack_lock, NULL) != 0 ||
        pthread_mutex_init(&self->_sack_lock, NULL) != 0)
        return pdp11_ram_uninit(&self->ram), RangeErr;

    pdp11_cpu_init(&self->cpu, &self->unibus, 0, (Pdp11CpuStat){0});

    unibus_init(
        &self->unibus,
        &self->cpu,
        pthread_mutex_ww_unibus_lock(&self->_sack_lock),
        pthread_mutex_ww_unibus_lock(&self->_bbsy_lock)
    );
    self->unibus.devices[0] = pdp11_ram_ww_unibus_device(&self->ram);

    self->_should_run = false;

    return Ok;
}
void pdp11_uninit(Pdp11 *const self) {
    if (self->_should_run) pdp11_power_down(self);

    pthread_mutex_destroy(&self->_bbsy_lock);
    pthread_mutex_destroy(&self->_sack_lock);

    pdp11_cpu_uninit(&self->cpu);
    pdp11_ram_uninit(&self->ram);
}

void pdp11_power_up(Pdp11 *const self) {
    self->_should_run = true;
    pthread_create(&self->_cpu_thread, NULL, pdp11_cpu_thread, self);
}
void pdp11_power_down(Pdp11 *const self) {
    // TODO? maybe cause power failure trap here?
    self->_should_run = false;
    pdp11_cpu_continue(&self->cpu);
    pthread_join(self->_cpu_thread, NULL);
}
