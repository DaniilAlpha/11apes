#include "test_unibus.h"

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

// TODO this is not working as interrupted cpu tries to write to stack via the
// unibus as well
static MiunteResult unibus_test_br() {
    MIUNTE_PASS();

    uint16_t const trap = 0xDEAD;
    MIUNTE_EXPECT(
        pdp11_cpu_pc(&pdp.cpu) != trap,
        "PC should not be on trap before BR"
    );
    unibus_br(&pdp.unibus, 07, trap);
    MIUNTE_EXPECT(
        pdp11_cpu_pc(&pdp.cpu) == trap,
        "PC should be on trap after BR"
    );
    MIUNTE_PASS();
}
static MiunteResult unibus_test_npr() {
    uint16_t const addr = 0x42;
    uint16_t const data = 0xF00D;
    unibus_dato(&pdp.unibus, addr, data);
    MIUNTE_EXPECT(
        unibus_dati(&pdp.unibus, addr) == data,
        "data should be written correctly"
    );
    unibus_datob(&pdp.unibus, addr + 1, (uint8_t)data);
    MIUNTE_EXPECT(
        unibus_dati(&pdp.unibus, addr) ==
            ((uint16_t)(data << 8) | (uint8_t)data),
        "data should be written correctly"
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
