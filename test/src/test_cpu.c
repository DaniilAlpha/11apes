#include "test_cpu.h"

#include <assert.h>
#include <miunte.h>

#include "pdp11/pdp11.h"

static Pdp11 pdp = {0};

/*************
 ** helpers **
 *************/

static uint16_t pdp11_cpu_dop_ra_instr(
    uint16_t const instr,
    uint16_t const x,
    uint16_t const y
) {
    unibus_dato(&pdp.unibus, pdp11_cpu_pc(&pdp.cpu), instr);

    pdp11_cpu_rx(&pdp.cpu, 0) = x;
    pdp11_cpu_rx(&pdp.cpu, 1) = y;
    pdp11_cpu_decode_exec(&pdp.cpu, pdp11_cpu_fetch(&pdp.cpu));
    return pdp11_cpu_rx(&pdp.cpu, 0);
}
static uint16_t pdp11_cpu_dop_da_instr(
    uint16_t const instr,
    uint16_t const addr,
    uint16_t const x,
    uint16_t const y
) {
    unibus_dato(&pdp.unibus, pdp11_cpu_pc(&pdp.cpu), instr);

    pdp11_cpu_rx(&pdp.cpu, 0) = addr;
    unibus_dato(&pdp.unibus, pdp11_cpu_rx(&pdp.cpu, 0), x);
    pdp11_cpu_rx(&pdp.cpu, 1) = y;
    pdp11_cpu_decode_exec(&pdp.cpu, pdp11_cpu_fetch(&pdp.cpu));
    return unibus_dati(&pdp.unibus, pdp11_cpu_rx(&pdp.cpu, 0));
}
static uint16_t pdp11_cpu_dop_ia_instr(
    uint16_t const instr,
    uint16_t const addr,
    uint16_t const off,
    uint16_t const x,
    uint16_t const y
) {
    unibus_dato(&pdp.unibus, pdp11_cpu_pc(&pdp.cpu), instr);
    unibus_dato(&pdp.unibus, pdp11_cpu_pc(&pdp.cpu) + 2, off);

    pdp11_cpu_rx(&pdp.cpu, 0) = addr;
    unibus_dato(&pdp.unibus, pdp11_cpu_rx(&pdp.cpu, 0) + off, x);
    pdp11_cpu_rx(&pdp.cpu, 1) = y;
    pdp11_cpu_decode_exec(&pdp.cpu, pdp11_cpu_fetch(&pdp.cpu));
    return unibus_dati(&pdp.unibus, pdp11_cpu_rx(&pdp.cpu, 0) + off);
}

/***********
 ** tests **
 ***********/

static MiunteResult pdp11_cpu_test_setup() {
    MIUNTE_EXPECT(pdp11_init(&pdp) == Ok, "`pdp11_init` should not fail");
    MIUNTE_PASS();
}
static MiunteResult pdp11_cpu_test_teardown() {
    pdp11_uninit(&pdp);
    MIUNTE_PASS();
}

static MiunteResult pdp11_cpu_test_addressing() {
    uint16_t const x = 0xDEAD, y = 0xBEEF;
    MIUNTE_EXPECT(x != y, "this just makes no sense");

    MIUNTE_EXPECT(
        pdp11_cpu_dop_ra_instr(0010100 /* mov R0, R1 */, x, y) == y,
        "register addressing should work correctly"
    );
    MIUNTE_EXPECT(
        pdp11_cpu_dop_da_instr(0010110 /* mov (R0), R1 */, 0x12, x, y) == y,
        "deferred addressing should work correctly"
    );
    MIUNTE_EXPECT(
        pdp11_cpu_dop_ia_instr(0010160 /* mov 24(R0), R1 */, 0x12, 42, x, y) ==
            y,
        "indexed deferred addressing should work correctly"
    );
    MIUNTE_PASS();
}

static MiunteResult pdp11_cpu_test_mov_movb() {
    Pdp11CpuStat *const stat = &pdp.cpu.stat;
    stat->cf = 1;

    {
        uint16_t const x = 1;

        MIUNTE_EXPECT(
            pdp11_cpu_dop_ra_instr(0010100 /* mov R0, R1 */, 0, x) == x,
            "mov should move correctly"
        );
        MIUNTE_EXPECT(
            !stat->nf && !stat->zf && !stat->vf,
            "mov of regular number should have {nzv} flags = {000}"
        );
    }
    MIUNTE_EXPECT(stat->cf == 1, "mov should not affect c flag");

    {
        uint16_t const x = 0;

        MIUNTE_EXPECT(
            pdp11_cpu_dop_ra_instr(0010100 /* mov R0, R1 */, 1, x) == x,
            "mov should move correctly"
        );
        MIUNTE_EXPECT(
            !stat->nf && stat->zf && !stat->vf,
            "mov of zero should have {nzv} flags = {010}"
        );
    }

    {
        uint16_t const x = -1;

        MIUNTE_EXPECT(
            pdp11_cpu_dop_ra_instr(0010100 /* mov R0, R1 */, 0, x) == x,
            "mov should move correctly"
        );
        MIUNTE_EXPECT(
            stat->nf && !stat->zf && !stat->vf,
            "mov of negative number should have {nzv} flags = {100}"
        );
    }

    MIUNTE_EXPECT(
        (int16_t)pdp11_cpu_dop_ra_instr(0110100 /* movb R1, R0 */, 0, -1) < 0,
        "movb to register should sign-extend the operand"
    );

    MIUNTE_PASS();
}
static MiunteResult pdp11_cpu_test_add_sub() {
    Pdp11CpuStat *const stat = &pdp.cpu.stat;
    {
        uint16_t const x = 3, y = 2;

        MIUNTE_EXPECT(
            pdp11_cpu_dop_ra_instr(0060100 /* add R0, R1 */, x, y) ==
                (uint16_t)(x + y),
            "add should add correctly"
        );
        MIUNTE_EXPECT(
            !stat->nf && !stat->zf && !stat->vf && !stat->cf,
            "add of regular result should result in {nzvc} flags = {0000}"
        );
    }

    {
        uint16_t const x = 3, y = -3;

        MIUNTE_EXPECT(
            pdp11_cpu_dop_ra_instr(0060100 /* add R0, R1 */, x, y) ==
                (uint16_t)(x + y),
            "add should add correctly"
        );
        MIUNTE_EXPECT(
            !stat->nf && stat->zf && !stat->vf && stat->cf,
            "add of zero result should result in {nzvc} flags = {0101}"
        );
    }

    {
        uint16_t const x = 3, y = -4;

        MIUNTE_EXPECT(
            pdp11_cpu_dop_ra_instr(0060100 /* add R0, R1 */, x, y) ==
                (uint16_t)(x + y),
            "add should add correctly"
        );
        MIUNTE_EXPECT(
            stat->nf && !stat->zf && !stat->vf && !stat->cf,
            "add of negative result should result in {nzvc} flags = {1000}"
        );
    }

    {
        uint16_t const x = 32000, y = 768;

        MIUNTE_EXPECT(
            pdp11_cpu_dop_ra_instr(0060100 /* add R0, R1 */, x, y) ==
                (uint16_t)(x + y),
            "add should add correctly"
        );
        MIUNTE_EXPECT(
            stat->nf && !stat->zf && stat->vf && !stat->cf,
            "add of overflow result should result in {nzvc} flags = {1010}"
        );
    }

    {
        uint16_t const x = 65000, y = 1000;

        MIUNTE_EXPECT(
            pdp11_cpu_dop_ra_instr(0060100 /* add R0, R1 */, x, y) ==
                (uint16_t)(x + y),
            "add should add correctly"
        );
        MIUNTE_EXPECT(
            !stat->nf && !stat->zf && !stat->vf && stat->cf,
            "add of unsigned overflow result should result in {nzvc} flags = {0001}"
        );
    }

    {
        uint16_t const x = 3, y = 3;

        MIUNTE_EXPECT(
            pdp11_cpu_dop_ra_instr(0160100 /* sub R0, R1 */, x, y) ==
                (uint16_t)(x - y),
            "sub should subtract correctly"
        );
        MIUNTE_EXPECT(
            !stat->nf && stat->zf && !stat->vf && !stat->cf,
            "sub of zero result should result in {nzvc} flags = {0100} (carry is opposite from add)"
        );
    }

    MIUNTE_PASS();
}
static MiunteResult pdp11_cpu_test_cmp() {
    Pdp11CpuStat *const stat = &pdp.cpu.stat;
    uint16_t const x = 24;

    pdp11_cpu_dop_ra_instr(0020001 /* cmp R0, R1 */, x, x);
    MIUNTE_EXPECT(
        !stat->nf && stat->zf && !stat->vf && !stat->cf,
        "cmp of equal values should result in {nzvc} flags = {0100}"
    );

    pdp11_cpu_dop_ra_instr(0020001 /* cmp R0, R1 */, x + 1, x);
    MIUNTE_EXPECT(
        !stat->nf && !stat->zf && !stat->vf && !stat->cf,
        "cmp of greater value should result in {nzvc} flags = {0000}"
    );

    pdp11_cpu_dop_ra_instr(0020001 /* cmp R0, R1 */, x, x + 1);
    MIUNTE_EXPECT(
        stat->nf && !stat->zf && !stat->vf && stat->cf,
        "cmp of lesser value should result in {nzvc} flags = {1001}"
    );

    MIUNTE_PASS();
}
static MiunteResult pdp11_cpu_test_mul_div() {
    Pdp11CpuStat *const stat = &pdp.cpu.stat;
    {
        uint16_t const x = 3, y = 2;

        MIUNTE_EXPECT(
            pdp11_cpu_dop_ra_instr(0070001 /* mul R0, R1 */, x, y) ==
                (uint16_t)(x * y),
            "mul should multiply correctly"
        );
        MIUNTE_EXPECT(
            !stat->nf && !stat->zf && !stat->vf && !stat->cf,
            "mul of regular result should result in {nzvc} flags = {0000}"
        );
    }

    {
        uint16_t const x = 3, y = 0;

        MIUNTE_EXPECT(
            pdp11_cpu_dop_ra_instr(0070001 /* mul R0, R1 */, x, y) ==
                (uint16_t)(x * y),
            "mul should multiply correctly"
        );
        MIUNTE_EXPECT(
            !stat->nf && stat->zf && !stat->vf && !stat->cf,
            "mul of zero result should result in {nzvc} flags = {0100}"
        );
    }

    {
        uint16_t const x = 3, y = -2;

        MIUNTE_EXPECT(
            pdp11_cpu_dop_ra_instr(0070001 /* mul R0, R1 */, x, y) ==
                (uint16_t)(x * y),
            "mul should multiply correctly"
        );
        MIUNTE_EXPECT(
            stat->nf && !stat->zf && !stat->vf && !stat->cf,
            "mul of negative result should result in {nzvc} flags = {1000}"
        );
    }

    {
        uint16_t const x = 32000, y = 2;

        MIUNTE_EXPECT(
            pdp11_cpu_dop_ra_instr(0070001 /* mul R0, R1 */, x, y) ==
                (uint16_t)(x * y),
            "mul should multiply correctly"
        );
        MIUNTE_EXPECT(
            stat->nf && !stat->zf && !stat->vf && stat->cf,
            "mul of overflow result should result in {nzvc} flags = {1001}"
        );
    }

    {
        uint16_t const x = 42;

        MIUNTE_EXPECT(
            pdp11_cpu_dop_ra_instr(0071001 /* div R0, R1 */, x, 0) == x,
            "div by zero should leave register untouched"
        );
        MIUNTE_EXPECT(
            stat->vf && stat->cf,
            "div by zero result should result in {vc} flags = {11}"
        );
    }

    MIUNTE_PASS();
}
static MiunteResult pdp11_cpu_test_bit() {
    Pdp11CpuStat *const stat = &pdp.cpu.stat;

    uint16_t const x = 0x42;

    MIUNTE_EXPECT(
        pdp11_cpu_dop_ra_instr(0030001 /* bit R0, R1 */, x, ~x) == x,
        "bit should not modify registers"
    );
    MIUNTE_EXPECT(
        !stat->nf && stat->zf && !stat->vf,
        "bit of zero result should result in {nzv} flags = {010}"
    );

    MIUNTE_PASS();
}

/**********
 ** main **
 **********/

int test_cpu_run(void) {
    MIUNTE_RUN(
        pdp11_cpu_test_setup,
        pdp11_cpu_test_teardown,
        {
            pdp11_cpu_test_addressing,

            // TODO test inc/dec
            // TODO test neg/com, especially flags

            // TODO test adc/sbc, especially flags

            // TODO test asr/asl
            // TODO test ror/rol

            pdp11_cpu_test_mov_movb,
            pdp11_cpu_test_add_sub,
            pdp11_cpu_test_cmp,
            pdp11_cpu_test_mul_div,
            pdp11_cpu_test_bit,

            // TODO test some of the branches
            // TODO test sob
        }
    );
}
