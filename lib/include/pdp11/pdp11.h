#ifndef PDP11_H
#define PDP11_H

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <assert.h>

#if (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
#  warning                                                                     \
      "This code was made to work on little-endian machines only, as PDP11 is little-endian itself."
#endif

#include <result.h>

#include "pdp11/cpu/pdp11_cpu.h"
#include "pdp11/pdp11_ram.h"

#define PDP11_STARTUP_PC (0100)
#define PDP11_STARTUP_CPU_STAT                                                 \
    ((Pdp11CpuStat){.priority = 0, .tf = 0, .nf = 0, .zf = 0, .vf = 0, .cf = 0})

typedef struct Pdp11 {
    Unibus unibus;

    Pdp11Cpu cpu;
    Pdp11Ram ram;

    pthread_t _cpu_thread;

    bool volatile _should_stop;
} Pdp11;

Result pdp11_init(Pdp11 *const self);
void pdp11_uninit(Pdp11 *const self);

void pdp11_start(Pdp11 *const self);
void pdp11_stop(Pdp11 *const self);

void pdp11_cause_power_fail(Pdp11 *const self);

#endif
