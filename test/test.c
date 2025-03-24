#include <miunte.h>

#include "pdp11/pdp11.h"

Pdp11 pdp = {0};
MiunteResult pdp_test_setup() {
    MIUNTE_EXPECT(pdp11_init(&pdp) == Ok, "`pdp11_init` should not fail");

    MIUNTE_PASS();
}
MiunteResult pdp_test_teardown() {
    pdp11_uninit(&pdp);

    MIUNTE_PASS();
}

MiunteResult pdp_test_addressing_w_xor() {
    uint16_t const x = 0b0011, y = 0b0101;

    {
        pdp.cpu.r[0] = x;
        pdp.cpu.r[1] = y;
        pdp11_ram_word_at(&pdp, pdp.cpu.r[7]) = 0074100 /* xor R0, R1 */;
        pdp11_step(&pdp);

        MIUNTE_EXPECT(
            pdp.cpu.r[0] == (x ^ y),
            "xor with register addressing should give the correct result"
        );
    }

    {
        pdp.cpu.r[0] = 0x0000, pdp11_ram_word_at(&pdp, pdp.cpu.r[0]) = x;
        pdp.cpu.r[1] = y;
        pdp11_ram_word_at(&pdp, pdp.cpu.r[7]) = 0074110;  // xor (R0), R1
        pdp11_step(&pdp);

        MIUNTE_EXPECT(
            pdp11_ram_word_at(&pdp, pdp.cpu.r[0]) == (x ^ y),
            "xor with deferred register addressing should give the correct result"
        );
    }

    {
        uint16_t const off = 24;

        pdp.cpu.r[0] = 0x0000, pdp11_ram_word_at(&pdp, pdp.cpu.r[0] + off) = x;
        pdp.cpu.r[1] = y;
        pdp11_ram_word_at(&pdp, pdp.cpu.r[7]) = 0074160,      // xor X(R0), R1
            pdp11_ram_word_at(&pdp, pdp.cpu.r[7] + 2) = off;  // X

        pdp11_step(&pdp);

        MIUNTE_EXPECT(
            pdp11_ram_word_at(&pdp, pdp.cpu.r[0] + off) == (x ^ y),
            "xor with deferred index addressing should give the correct result"
        );
    }

    MIUNTE_PASS();
}

int main() {
    MIUNTE_RUN(
        pdp_test_setup,
        pdp_test_teardown,
        {
            pdp_test_addressing_w_xor,
        }
    );
}
