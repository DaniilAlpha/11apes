#include <assert.h>

#define MIUNTE_STOP_ON_FAILURE (1)
#include <miunte.h>

#include "conviniences.h"
#include "pdp11/pdp11.h"

static_assert(bits(0x1120, 0, 3) == 0, "bits macro should work as expected");
static_assert(bits(0x1120, 4, 7) == 2, "bits macro should work as expected");
static_assert(bits(0x1120, 0, 7) == 0x20, "bits macro should work as expected");
static_assert(bits(0x1120, 12, 15) == 1, "bits macro should work as expected");

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
static MiunteResult pdp_test_add() {
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
            "add of regular result should have {nzvc} flags = {0000}"
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
            !ps->nf && ps->zf && !ps->vf && !ps->cf,
            "add of zero result should have {nzvc} flags = {0100}"
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
            "add of negative result should have {nzvc} flags = {1000}"
        );
    }

    {
        uint16_t const x = 32000, y = 769;

        MIUNTE_EXPECT(
            pdp_test_dop_ra_instr(0060100 /* add R0, R1 */, x, y) ==
                (uint16_t)(x + y),
            "add should add correctly"
        );
        MIUNTE_EXPECT(
            ps->nf && !ps->zf && ps->vf && !ps->cf,
            "add of overflow result should have {nzvc} flags = {1010}"
        );
    }

    {
        uint16_t const x = 65000, y = 536;

        MIUNTE_EXPECT(
            pdp_test_dop_ra_instr(0060100 /* add R0, R1 */, x, y) ==
                (uint16_t)(x + y),
            "add should add correctly"
        );
        MIUNTE_EXPECT(
            !ps->nf && !ps->zf && !ps->vf && ps->cf,
            "add of unsigned overflow result should have {nzvc} flags = {0001}"
        );
    }

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
            pdp_test_addressing,

            pdp_test_mov_movb,
            pdp_test_add,
            pdp_test_cmp,  // TODO better test cmp flags

            // TODO test clr flags
            // TODO test inc/dec flags
            // TODO test tst flags
            // TODO test neg/com flags
            // TODO test mov flags
            // TODO test mul flags
            // TODO test div flags
            // TODO test bit flags
            // TODO test some of the branches
            //
            // TODO test neg, as not sure in the rightness of the implon
            // TODO check if flags are correctly set on core set of arith ops
        }
    );
}
