#include "test_unibus.h"

#include "miunte.h"

// Pdp11
static MiunteResult pdp_test_dati() { MIUNTE_PASS(); }

/**********
 ** main **
 **********/

int test_unibus_run(void) {
    MIUNTE_RUN(
        NULL,
        NULL,
        {
            pdp_test_dati,
        }
    );
}
