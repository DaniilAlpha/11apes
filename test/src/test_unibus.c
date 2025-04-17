#include "test_unibus.h"

#include <unistd.h>

#include "miunte.h"
#include "pdp11/pdp11.h"

static Pdp11 pdp = {0};

/***********
 ** tests **
 ***********/

static MiunteResult unibus_test_setup() {
    MIUNTE_EXPECT(pdp11_init(&pdp) == Ok, "`pdp11_init` should not fail");
    MIUNTE_PASS();
}
static MiunteResult unibus_test_teardown() {
    pdp11_uninit(&pdp);
    MIUNTE_PASS();
}

static MiunteResult unibus_test_npr() {
    UnibusDevice const *const device =
        &pdp.unibus.devices[PDP11_FIRST_USER_DEVICE];

    uint16_t const addr = 0x42;
    uint16_t const data = 0xF00D;
    (unibus_npr(&pdp.unibus, device),
     unibus_dato(&pdp.unibus, device, addr, data));
    MIUNTE_EXPECT(
        unibus_dati(&pdp.unibus, device, addr) == data,
        "data should be written correctly"
    );
    (unibus_npr(&pdp.unibus, device),
     unibus_datob(&pdp.unibus, device, addr + 1, (uint8_t)data));
    MIUNTE_EXPECT(
        (unibus_npr(&pdp.unibus, device), unibus_dati(&pdp.unibus, device, addr)
        ) == ((uint16_t)(data << 8) | (uint8_t)data),
        "data should be written correctly"
    );
    MIUNTE_PASS();
}

static void *lower_cpu_priority_thread(void *const vcpu) {
    Pdp11Cpu *const cpu = vcpu;

    while (--pdp11_cpu_stat(cpu).priority > 0) usleep(10 * 1000);

    return NULL;
}
static MiunteResult unibus_test_br() {
    UnibusDevice const *const device =
        &pdp.unibus.devices[PDP11_FIRST_USER_DEVICE];

    uint16_t const trap = 0x42, trap_pc = 0xACE;
    (unibus_npr(&pdp.unibus, device),
     unibus_dato(&pdp.unibus, device, trap, trap_pc));

    MIUNTE_EXPECT(
        pdp11_cpu_pc(&pdp.cpu) != trap_pc,
        "PC should not be on trap before BR"
    );

    pdp11_cpu_stat(&pdp.cpu).priority = 07;
    pthread_t thread;
    pthread_create(&thread, NULL, lower_cpu_priority_thread, &pdp.cpu);
    MIUNTE_EXPECT(
        ((Pdp11CpuStat volatile)pdp11_cpu_stat(&pdp.cpu)).priority >= 03,
        "test will be more useful if starting priority is greater than that of an interrupt"
    );

    (unibus_br(&pdp.unibus, device, 03),
     unibus_intr(&pdp.unibus, device, trap));

    MIUNTE_EXPECT(
        ((Pdp11CpuStat volatile)pdp11_cpu_stat(&pdp.cpu)).priority < 03,
        "interrupt should have happened at priority level lower than that of an interrupt"
    );
    pthread_cancel(thread);

    MIUNTE_EXPECT(
        pdp11_cpu_pc(&pdp.cpu) == trap_pc,
        "PC should be on trap after BR"
    );

    MIUNTE_PASS();
}

/**********
 ** main **
 **********/

int test_unibus_run(void) {
    MIUNTE_RUN(
        unibus_test_setup,
        unibus_test_teardown,
        {
            unibus_test_br,
            unibus_test_npr,
        }
    );
}
