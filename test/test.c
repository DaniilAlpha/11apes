#include <miunte.h>

#include "pdp11/pdp11.h"

Pdp11 pdp = {0};

/*************
 ** helpers **
 *************/

static uint16_t pdp_test_dop_ra_instr(
    uint16_t const instr,
    uint16_t const x,
    uint16_t const y
) {
    pdp11_rx(&pdp, 0) = x;
    pdp11_rx(&pdp, 1) = y;
    pdp11_ram_word_at(&pdp, pdp11_pc(&pdp)) = instr;
    pdp11_step(&pdp);
    return pdp11_rx(&pdp, 0);
}
static uint16_t pdp_test_dop_da_instr(
    uint16_t const instr,
    uint16_t const addr,
    uint16_t const x,
    uint16_t const y
) {
    pdp11_rx(&pdp, 0) = addr, pdp11_ram_word_at(&pdp, addr) = x;
    pdp11_rx(&pdp, 1) = y;
    pdp11_ram_word_at(&pdp, pdp11_pc(&pdp)) = instr;
    pdp11_step(&pdp);
    return pdp11_ram_word_at(&pdp, addr);
}
static uint16_t pdp_test_dop_ia_instr(
    uint16_t const instr,
    uint16_t const addr,
    uint16_t const off,
    uint16_t const x,
    uint16_t const y
) {
    pdp11_rx(&pdp, 0) = addr, pdp11_ram_word_at(&pdp, addr + off) = x;
    pdp11_rx(&pdp, 1) = y;
    pdp11_ram_word_at(&pdp, pdp11_pc(&pdp)) = instr;
    pdp11_ram_word_at(&pdp, pdp11_pc(&pdp) + 2) = off;
    pdp11_step(&pdp);
    return pdp11_ram_word_at(&pdp, addr + off);
}

/***********
 ** tests **
 ***********/

static MiunteResult pdp_test_setup() {
    MIUNTE_EXPECT(pdp11_init(&pdp) == Ok, "`pdp11_init` should not fail");
    MIUNTE_PASS();
}
static MiunteResult pdp_test_teardown() {
    pdp11_uninit(&pdp);
    MIUNTE_PASS();
}

static MiunteResult pdp_test_addressing_w_xor() {
    uint16_t const x = 0b0011, y = 0b0101;

    MIUNTE_EXPECT(
        pdp_test_dop_ra_instr(0074100 /* xor R0, R1 */, x, y) == (x ^ y),
        "xor with register addressing should give the correct result"
    );
    MIUNTE_EXPECT(
        pdp_test_dop_da_instr(0074110 /* xor (R0), R1 */, 0x0042, x, y) ==
            (x ^ y),
        "xor with deferred addressing should give the correct result"
    );
    MIUNTE_EXPECT(
        pdp_test_dop_ia_instr(0074160 /* xor 24(R0), R1 */, 0x0042, 24, x, y) ==
            (x ^ y),
        "xor with deferred index addressing should give the correct result"
    );
    MIUNTE_PASS();
}

static MiunteResult pdp_test_movb() {
    uint16_t const x = -123;

    MIUNTE_EXPECT(
        pdp_test_dop_ra_instr(0110100 /* movb R1, R0 */, 0, x & 0xFF) == x,
        "movb to register should sign-extend the operand"
    );

    MIUNTE_PASS();
}

static MiunteResult pdp_test_cmp() {
    uint16_t const x = 24;

    pdp_test_dop_ra_instr(0020001 /* cmp R0, R1 */, x, x);
    MIUNTE_EXPECT(pdp11_ps(&pdp).zf == 1, "cmp of equal values should set ZF");

    pdp_test_dop_ra_instr(0020001 /* cmp R0, R1 */, x + 1, x);
    MIUNTE_EXPECT(
        (pdp11_ps(&pdp).nf ^ pdp11_ps(&pdp).vf) == 0,
        "cmp of greater value should set NF and VF to the same value"
    );

    pdp_test_dop_ra_instr(0020001 /* cmp R0, R1 */, x, x + 1);
    MIUNTE_EXPECT(
        (pdp11_ps(&pdp).nf ^ pdp11_ps(&pdp).vf) == 1,
        "cmp of lesser value should set NF and VF to the different value"
    );

    MIUNTE_PASS();
}

/**********
 ** main **
 **********/

int main() {
    MIUNTE_RUN(
        pdp_test_setup,
        pdp_test_teardown,
        {
            pdp_test_addressing_w_xor,
            pdp_test_movb,
            pdp_test_cmp,
        }
    );
}
