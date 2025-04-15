#ifndef PDP11_H
#define PDP11_H
#if (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
#  warning                                                                     \
      "This code was made to work on little-endian machines only, as PDP11 is little-endian itself."
#endif

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <assert.h>
#include <result.h>

#include "pdp11/cpu/pdp11_cpu.h"
#include "pdp11/pdp11_ram.h"

#define PDP11_STARTUP_PC (0100)
#define PDP11_STARTUP_CPU_STAT                                                 \
    ((Pdp11CpuStat){.priority = 0, .tf = 0, .nf = 0, .zf = 0, .vf = 0, .cf = 0})

#define PDP11_FIRST_USER_DEVICE (1)

typedef struct Pdp11 {
    Unibus unibus;

    Pdp11Cpu cpu;
    Pdp11Ram ram;

    pthread_t _cpu_thread;
    pthread_mutex_t _sack_lock, _bbsy_lock;

    bool volatile _should_run;
} Pdp11;

Result pdp11_init(Pdp11 *const self);
void pdp11_uninit(Pdp11 *const self);

void pdp11_start(Pdp11 *const self);
void pdp11_stop(Pdp11 *const self);

#endif
