#include <assert.h>

#define MIUNTE_STOP_ON_FAILURE (1)
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
    pdp11_rx(&pdp, 0) = addr, pdp11_ram_word_at(&pdp, pdp11_rx(&pdp, 0)) = x;
    pdp11_rx(&pdp, 1) = y;
    pdp11_ram_word_at(&pdp, pdp11_pc(&pdp)) = instr;
    pdp11_step(&pdp);
    return pdp11_ram_word_at(&pdp, pdp11_rx(&pdp, 0));
}
static uint16_t pdp_test_dop_ia_instr(
    uint16_t const instr,
    uint16_t const addr,
    uint16_t const off,
    uint16_t const x,
    uint16_t const y
) {
    pdp11_rx(&pdp, 0) = addr,
                   pdp11_ram_word_at(&pdp, pdp11_rx(&pdp, 0) + off) = x;
    pdp11_rx(&pdp, 1) = y;
    pdp11_ram_word_at(&pdp, pdp11_pc(&pdp)) = instr;
    pdp11_ram_word_at(&pdp, pdp11_pc(&pdp) + 2) = off;
    pdp11_step(&pdp);
    return pdp11_ram_word_at(&pdp, pdp11_rx(&pdp, 0) + off);
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

static MiunteResult pdp_test_addressing() {
    uint16_t const x = 0xDEAD, y = 0xBEEF;
    static_assert(x != y, "this just makes no sense");

    MIUNTE_EXPECT(
        pdp_test_dop_ra_instr(0010100 /* mov R0, R1 */, x, y) == y,
        "register addressing should work correctly"
    );
    MIUNTE_EXPECT(
        pdp_test_dop_da_instr(0010110 /* mov (R0), R1 */, 0x0042, x, y) == y,
        "deferred addressing should work correctly"
    );
    MIUNTE_EXPECT(
        pdp_test_dop_ia_instr(0010160 /* mov 24(R0), R1 */, 0x0042, 24, x, y) ==
            y,
        "indexed deferred addressing should work correctly"
    );
    MIUNTE_PASS();
}

static MiunteResult pdp_test_mov_movb() {
    Pdp11Ps *const ps = &pdp11_ps(&pdp);
    ps->cf = 1;

    {
        uint16_t const x = 1;

        MIUNTE_EXPECT(
            pdp_test_dop_ra_instr(0010100 /* mov R0, R1 */, 0, x) == x,
            "mov should move correctly"
        );
        MIUNTE_EXPECT(
            !ps->nf && !ps->zf && !ps->vf,
            "mov of regular number should have {nzv} flags = {000}"
        );
    }
    MIUNTE_EXPECT(ps->cf == 1, "mov should not affect c flag");

    {
        uint16_t const x = 0;

        MIUNTE_EXPECT(
            pdp_test_dop_ra_instr(0010100 /* mov R0, R1 */, 1, x) == x,
            "mov should move correctly"
        );
        MIUNTE_EXPECT(
            !ps->nf && ps->zf && !ps->vf,
            "mov of zero should have {nzv} flags = {010}"
        );
    }

    {
        uint16_t const x = -1;

        MIUNTE_EXPECT(
            pdp_test_dop_ra_instr(0010100 /* mov R0, R1 */, 0, x) == x,
            "mov should move correctly"
        );
        MIUNTE_EXPECT(
            ps->nf && !ps->zf && !ps->vf,
            "mov of negative number should have {nzv} flags = {100}"
        );
    }

    MIUNTE_EXPECT(
        (int16_t)pdp_test_dop_ra_instr(0110100 /* movb R1, R0 */, 0, -1) < 0,
        "movb to register should sign-extend the operand"
    );

    MIUNTE_PASS();
}
static MiunteResult pdp_test_add_sub() {
    Pdp11Ps *const ps = &pdp11_ps(&pdp);

    {
        uint16_t const x = 3, y = 2;

        MIUNTE_EXPECT(
            pdp_test_dop_ra_instr(0060100 /* add R0, R1 */, x, y) ==
                (uint16_t)(x + y),
            "add should add correctly"
        );
        MIUNTE_EXPECT(
            !ps->nf && !ps->zf && !ps->vf && !ps->cf,
            "add of regular result should result in {nzvc} flags = {0000}"
        );
    }

    {
        uint16_t const x = 3, y = -3;

        MIUNTE_EXPECT(
            pdp_test_dop_ra_instr(0060100 /* add R0, R1 */, x, y) ==
                (uint16_t)(x + y),
            "add should add correctly"
        );
        MIUNTE_EXPECT(
            !ps->nf && ps->zf && !ps->vf && ps->cf,
            "add of zero result should result in {nzvc} flags = {0101}"
        );
    }

    {
        uint16_t const x = 3, y = -4;

        MIUNTE_EXPECT(
            pdp_test_dop_ra_instr(0060100 /* add R0, R1 */, x, y) ==
                (uint16_t)(x + y),
            "add should add correctly"
        );
        MIUNTE_EXPECT(
            ps->nf && !ps->zf && !ps->vf && !ps->cf,
            "add of negative result should result in {nzvc} flags = {1000}"
        );
    }

    {
        uint16_t const x = 32000, y = 768;

        MIUNTE_EXPECT(
            pdp_test_dop_ra_instr(0060100 /* add R0, R1 */, x, y) ==
                (uint16_t)(x + y),
            "add should add correctly"
        );
        MIUNTE_EXPECT(
            ps->nf && !ps->zf && ps->vf && !ps->cf,
            "add of overflow result should result in {nzvc} flags = {1010}"
        );
    }

    {
        uint16_t const x = 65000, y = 1000;

        MIUNTE_EXPECT(
            pdp_test_dop_ra_instr(0060100 /* add R0, R1 */, x, y) ==
                (uint16_t)(x + y),
            "add should add correctly"
        );
        MIUNTE_EXPECT(
            !ps->nf && !ps->zf && !ps->vf && ps->cf,
            "add of unsigned overflow result should result in {nzvc} flags = {0001}"
        );
    }

    {
        uint16_t const x = 3, y = 3;

        MIUNTE_EXPECT(
            pdp_test_dop_ra_instr(0160100 /* sub R0, R1 */, x, y) ==
                (uint16_t)(x - y),
            "sub should subtract correctly"
        );
        MIUNTE_EXPECT(
            !ps->nf && ps->zf && !ps->vf && !ps->cf,
            "sub of zero result should result in {nzvc} flags = {0100} (carry is opposite from add)"
        );
    }

    MIUNTE_PASS();
}
static MiunteResult pdp_test_cmp() {
    Pdp11Ps *const ps = &pdp11_ps(&pdp);

    uint16_t const x = 24;

    pdp_test_dop_ra_instr(0020001 /* cmp R0, R1 */, x, x);
    MIUNTE_EXPECT(
        !ps->nf && ps->zf && !ps->vf && !ps->cf,
        "cmp of equal values should result in {nzvc} flags = {0100}"
    );

    pdp_test_dop_ra_instr(0020001 /* cmp R0, R1 */, x + 1, x);
    MIUNTE_EXPECT(
        !ps->nf && !ps->zf && !ps->vf && !ps->cf,
        "cmp of greater value should result in {nzvc} flags = {0000}"
    );

    pdp_test_dop_ra_instr(0020001 /* cmp R0, R1 */, x, x + 1);
    MIUNTE_EXPECT(
        ps->nf && !ps->zf && !ps->vf && ps->cf,
        "cmp of lesser value should result in {nzvc} flags = {1001}"
    );

    MIUNTE_PASS();
}
static MiunteResult pdp_test_mul_div() {
    Pdp11Ps *const ps = &pdp11_ps(&pdp);

    {
        uint16_t const x = 3, y = 2;

        MIUNTE_EXPECT(
            pdp_test_dop_ra_instr(0070001 /* mul R0, R1 */, x, y) ==
                (uint16_t)(x * y),
            "mul should multiply correctly"
        );
        MIUNTE_EXPECT(
            !ps->nf && !ps->zf && !ps->vf && !ps->cf,
            "mul of regular result should result in {nzvc} flags = {0000}"
        );
    }

    {
        uint16_t const x = 3, y = 0;

        MIUNTE_EXPECT(
            pdp_test_dop_ra_instr(0070001 /* mul R0, R1 */, x, y) ==
                (uint16_t)(x * y),
            "mul should multiply correctly"
        );
        MIUNTE_EXPECT(
            !ps->nf && ps->zf && !ps->vf && !ps->cf,
            "mul of zero result should result in {nzvc} flags = {0100}"
        );
    }

    {
        uint16_t const x = 3, y = -2;

        MIUNTE_EXPECT(
            pdp_test_dop_ra_instr(0070001 /* mul R0, R1 */, x, y) ==
                (uint16_t)(x * y),
            "mul should multiply correctly"
        );
        MIUNTE_EXPECT(
            ps->nf && !ps->zf && !ps->vf && !ps->cf,
            "mul of negative result should result in {nzvc} flags = {1000}"
        );
    }

    {
        uint16_t const x = 32000, y = 2;

        MIUNTE_EXPECT(
            pdp_test_dop_ra_instr(0070001 /* mul R0, R1 */, x, y) ==
                (uint16_t)(x * y),
            "mul should multiply correctly"
        );
        MIUNTE_EXPECT(
            ps->nf && !ps->zf && !ps->vf && ps->cf,
            "mul of overflow result should result in {nzvc} flags = {1001}"
        );
    }

    {
        uint16_t const x = 42;

        MIUNTE_EXPECT(
            pdp_test_dop_ra_instr(0071001 /* div R0, R1 */, x, 0) == x,
            "div by zero should leave register untouched"
        );
        MIUNTE_EXPECT(
            ps->vf && ps->cf,
            "div by zero result should result in {vc} flags = {11}"
        );
    }

    MIUNTE_PASS();
}
static MiunteResult pdp_test_bit() {
    Pdp11Ps *const ps = &pdp11_ps(&pdp);

    uint16_t const x = 0x42;

    MIUNTE_EXPECT(
        pdp_test_dop_ra_instr(0030001 /* bit R0, R1 */, x, ~x) == x,
        "bit should not modify registers"
    );
    MIUNTE_EXPECT(
        !ps->nf && ps->zf && !ps->vf,
        "bit of zero result should result in {nzv} flags = {010}"
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
            pdp_test_addressing,

            pdp_test_mov_movb,
            pdp_test_add_sub,
            pdp_test_cmp,
            pdp_test_mul_div,
            pdp_test_bit,
            // TODO test some of the branches
            // TODO test sob

            // TODO test clr flags
            // TODO test inc/dec flags
            // TODO test tst flags
            // TODO test neg/com flags

            // TODO test neg, as not sure in the rightness of the implon
        }
    );
}
