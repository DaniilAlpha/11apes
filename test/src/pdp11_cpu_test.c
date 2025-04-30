#include "pdp11_cpu_test.h"

#include <assert.h>
#include <miunte.h>
#include <unistd.h>

#include "pdp11/cpu/pdp11_cpu_instr.h"
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
    unibus_cpu_dato(&pdp.unibus, pdp11_cpu_pc(&pdp.cpu), instr);

    pdp11_cpu_rx(&pdp.cpu, 0) = x;
    pdp11_cpu_rx(&pdp.cpu, 1) = y;
    pdp11_cpu_single_step(&pdp.cpu);
    while (pdp11_cpu_state(&pdp.cpu) != PDP11_CPU_STATE_HALT) sleep(0);
    return pdp11_cpu_rx(&pdp.cpu, 0);
}
static uint16_t pdp11_cpu_dop_da_instr(
    uint16_t const instr,
    uint16_t const addr,
    uint16_t const x,
    uint16_t const y
) {
    unibus_cpu_dato(&pdp.unibus, pdp11_cpu_pc(&pdp.cpu), instr);

    pdp11_cpu_rx(&pdp.cpu, 0) = addr;
    unibus_cpu_dato(&pdp.unibus, pdp11_cpu_rx(&pdp.cpu, 0), x);
    pdp11_cpu_rx(&pdp.cpu, 1) = y;
    pdp11_cpu_single_step(&pdp.cpu);
    while (pdp11_cpu_state(&pdp.cpu) != PDP11_CPU_STATE_HALT) sleep(0);
    uint16_t res;
    return unibus_cpu_dati(&pdp.unibus, pdp11_cpu_rx(&pdp.cpu, 0), &res), res;
}
static uint16_t pdp11_cpu_dop_ia_instr(
    uint16_t const instr,
    uint16_t const addr,
    uint16_t const off,
    uint16_t const x,
    uint16_t const y
) {
    unibus_cpu_dato(&pdp.unibus, pdp11_cpu_pc(&pdp.cpu), instr);
    unibus_cpu_dato(&pdp.unibus, pdp11_cpu_pc(&pdp.cpu) + 2, off);

    pdp11_cpu_rx(&pdp.cpu, 0) = addr;
    unibus_cpu_dato(&pdp.unibus, pdp11_cpu_rx(&pdp.cpu, 0) + off, x);
    pdp11_cpu_rx(&pdp.cpu, 1) = y;
    pdp11_cpu_single_step(&pdp.cpu);
    while (pdp11_cpu_state(&pdp.cpu) != PDP11_CPU_STATE_HALT) sleep(0);
    uint16_t res;
    return unibus_cpu_dati(&pdp.unibus, pdp11_cpu_rx(&pdp.cpu, 0) + off, &res),
           res;
}

/***********
 ** tests **
 ***********/

static MiunteResult pdp11_cpu_test_setup() {
    MIUNTE_EXPECT(pdp11_init(&pdp) == Ok, "`pdp11_init` should not fail");
    pdp11_cpu_pc(&pdp.cpu) = 0x100;
    pdp11_cpu_sp(&pdp.cpu) = 0x1000;
    MIUNTE_PASS();
}
static MiunteResult pdp11_cpu_test_teardown() {
    pdp11_uninit(&pdp);
    MIUNTE_PASS();
}

static MiunteResult pdp11_cpu_test_decoding_and_execution() {
    {
        uint16_t const encoded = 0010100;

        Pdp11CpuInstr const instr = pdp11_cpu_instr(encoded);
        MIUNTE_EXPECT(
            instr.type == PDP11_CPU_INSTR_TYPE_OO,
            "type of MOV should be decoded OO"
        );
        MIUNTE_EXPECT(
            instr.u.oo.o0 == 001,
            "operand 0 should be decoded correctly"
        );
        MIUNTE_EXPECT(
            instr.u.oo.o1 == 000,
            "operand 1 should be decoded correctly"
        );

        unibus_cpu_dato(&pdp.unibus, pdp11_cpu_pc(&pdp.cpu), encoded);
        pdp11_cpu_single_step(&pdp.cpu);
        while (pdp11_cpu_state(&pdp.cpu) != PDP11_CPU_STATE_HALT) sleep(0);
    }
    {
        uint16_t const reserved_instr_trap = 0xFACE;
        unibus_cpu_dato(
            &pdp.unibus,
            PDP11_CPU_TRAP_RESERVED_INSTR,
            reserved_instr_trap
        );

        uint16_t const encoded = 0177000;

        Pdp11CpuInstr const instr = pdp11_cpu_instr(encoded);
        MIUNTE_EXPECT(
            instr.type == PDP11_CPU_INSTR_TYPE_RESERVED,
            "illegal instruction should result in an illegal type"
        );

        MIUNTE_EXPECT(
            pdp11_cpu_pc(&pdp.cpu) != reserved_instr_trap,
            "before executing an illegal instruction should not be on illegal instr location"
        );
        unibus_cpu_dato(&pdp.unibus, pdp11_cpu_pc(&pdp.cpu), encoded);
        pdp11_cpu_single_step(&pdp.cpu);
        while (pdp11_cpu_state(&pdp.cpu) != PDP11_CPU_STATE_HALT) sleep(0);
        MIUNTE_EXPECT(
            pdp11_cpu_pc(&pdp.cpu) == reserved_instr_trap,
            "after executing an illegal instruction should trap to illegal instr location"
        );
    }

    MIUNTE_PASS();
}

static MiunteResult pdp11_cpu_test_addressing() {
    {
        uint16_t const x = 0xDEAD, y = 0xBEEF;
        MIUNTE_EXPECT(x != y, "this just makes no sense");

        MIUNTE_EXPECT(
            pdp11_cpu_dop_ra_instr(0010100 /* mov R0, R1 */, x, y) == y,
            "register addressing should work correctly"
        );
        MIUNTE_EXPECT(
            pdp11_cpu_dop_da_instr(0010110 /* mov (R0), R1 */, 0x112, x, y) ==
                y,
            "deferred addressing should work correctly"
        );
        MIUNTE_EXPECT(
            pdp11_cpu_dop_ia_instr(
                0010160 /* mov 24(R0), R1 */,
                0x112,
                42,
                x,
                y
            ) == y,
            "indexed deferred addressing should work correctly"
        );
    }
    {
        uint16_t const cpu_err_trap = 0xFACE;
        unibus_cpu_dato(&pdp.unibus, PDP11_CPU_TRAP_CPU_ERR, cpu_err_trap);

        pdp11_cpu_rx(&pdp.cpu, 0) = PDP11_RAM_SIZE;
        // pdp11_cpu_pc(&pdp.cpu) = 0x100;

        MIUNTE_EXPECT(
            pdp11_cpu_pc(&pdp.cpu) != cpu_err_trap,
            "before timeout error (accessing illegal address) should not trap to cpu err location"
        );
        unibus_cpu_dato(
            &pdp.unibus,
            pdp11_cpu_pc(&pdp.cpu),
            0010110 /* mov (R0), R1 */
        );
        pdp11_cpu_single_step(&pdp.cpu);
        while (pdp11_cpu_state(&pdp.cpu) != PDP11_CPU_STATE_HALT) sleep(0);
        MIUNTE_EXPECT(
            pdp11_cpu_pc(&pdp.cpu) == cpu_err_trap,
            "after timeout error (accessing illegal address) should trap to cpu err location"
        );

        pdp11_cpu_pc(&pdp.cpu) = 0x100;

        MIUNTE_EXPECT(
            pdp11_cpu_pc(&pdp.cpu) != cpu_err_trap,
            "before timeout error (accessing illegal address) should not trap to cpu err location"
        );
        unibus_cpu_dato(
            &pdp.unibus,
            pdp11_cpu_pc(&pdp.cpu),
            0010130 /* mov @(R0)+, R1 */
        );
        pdp11_cpu_single_step(&pdp.cpu);
        while (pdp11_cpu_state(&pdp.cpu) != PDP11_CPU_STATE_HALT) sleep(0);
        MIUNTE_EXPECT(
            pdp11_cpu_pc(&pdp.cpu) == cpu_err_trap,
            "after timeout error (accessing illegal address) should trap to cpu err location"
        );
    }

    MIUNTE_PASS();
}

static MiunteResult pdp11_cpu_test_mov_movb() {
    Pdp11Psw volatile *const psw = &pdp11_cpu_psw(&pdp.cpu);
    psw->cf = 1;

    {
        uint16_t const x = 1;

        MIUNTE_EXPECT(
            pdp11_cpu_dop_ra_instr(0010100 /* mov R0, R1 */, 0, x) == x,
            "mov should move correctly"
        );
        MIUNTE_EXPECT(
            !psw->nf && !psw->zf && !psw->vf,
            "mov of regular number should have {nzv} flags = {000}"
        );
    }
    MIUNTE_EXPECT(psw->cf == 1, "mov should not affect c flag");

    {
        uint16_t const x = 0;

        MIUNTE_EXPECT(
            pdp11_cpu_dop_ra_instr(0010100 /* mov R0, R1 */, 1, x) == x,
            "mov should move correctly"
        );
        MIUNTE_EXPECT(
            !psw->nf && psw->zf && !psw->vf,
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
            psw->nf && !psw->zf && !psw->vf,
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
    Pdp11Psw volatile *const psw = &pdp11_cpu_psw(&pdp.cpu);
    {
        uint16_t const x = 3, y = 2;

        MIUNTE_EXPECT(
            pdp11_cpu_dop_ra_instr(0060100 /* add R0, R1 */, x, y) ==
                (uint16_t)(x + y),
            "add should add correctly"
        );
        MIUNTE_EXPECT(
            !psw->nf && !psw->zf && !psw->vf && !psw->cf,
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
            !psw->nf && psw->zf && !psw->vf && psw->cf,
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
            psw->nf && !psw->zf && !psw->vf && !psw->cf,
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
            psw->nf && !psw->zf && psw->vf && !psw->cf,
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
            !psw->nf && !psw->zf && !psw->vf && psw->cf,
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
            !psw->nf && psw->zf && !psw->vf && !psw->cf,
            "sub of zero result should result in {nzvc} flags = {0100} (carry is opposite from add)"
        );
    }

    MIUNTE_PASS();
}
static MiunteResult pdp11_cpu_test_cmp() {
    Pdp11Psw volatile *const psw = &pdp11_cpu_psw(&pdp.cpu);
    uint16_t const x = 24;

    pdp11_cpu_dop_ra_instr(0020001 /* cmp R0, R1 */, x, x);
    MIUNTE_EXPECT(
        !psw->nf && psw->zf && !psw->vf && !psw->cf,
        "cmp of equal values should result in {nzvc} flags = {0100}"
    );

    pdp11_cpu_dop_ra_instr(0020001 /* cmp R0, R1 */, x + 1, x);
    MIUNTE_EXPECT(
        !psw->nf && !psw->zf && !psw->vf && !psw->cf,
        "cmp of greater value should result in {nzvc} flags = {0000}"
    );

    pdp11_cpu_dop_ra_instr(0020001 /* cmp R0, R1 */, x, x + 1);
    MIUNTE_EXPECT(
        psw->nf && !psw->zf && !psw->vf && psw->cf,
        "cmp of lesser value should result in {nzvc} flags = {1001}"
    );

    MIUNTE_PASS();
}
static MiunteResult pdp11_cpu_test_mul_div() {
    Pdp11Psw volatile *const psw = &pdp11_cpu_psw(&pdp.cpu);
    {
        uint16_t const x = 3, y = 2;

        MIUNTE_EXPECT(
            pdp11_cpu_dop_ra_instr(0070001 /* mul R0, R1 */, x, y) ==
                (uint16_t)(x * y),
            "mul should multiply correctly"
        );
        MIUNTE_EXPECT(
            !psw->nf && !psw->zf && !psw->vf && !psw->cf,
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
            !psw->nf && psw->zf && !psw->vf && !psw->cf,
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
            psw->nf && !psw->zf && !psw->vf && !psw->cf,
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
            psw->nf && !psw->zf && !psw->vf && psw->cf,
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
            psw->vf && psw->cf,
            "div by zero result should result in {vc} flags = {11}"
        );
    }

    MIUNTE_PASS();
}
static MiunteResult pdp11_cpu_test_bit() {
    Pdp11Psw volatile *const psw = &pdp11_cpu_psw(&pdp.cpu);

    uint16_t const x = 0x42;

    MIUNTE_EXPECT(
        pdp11_cpu_dop_ra_instr(0030001 /* bit R0, R1 */, x, ~x) == x,
        "bit should not modify registers"
    );
    MIUNTE_EXPECT(
        !psw->nf && psw->zf && !psw->vf,
        "bit of zero result should result in {nzv} flags = {010}"
    );

    MIUNTE_PASS();
}
static MiunteResult pdp11_cpu_test_swab() {
    Pdp11Psw volatile *const psw = &pdp11_cpu_psw(&pdp.cpu);

    uint16_t const x = 0x12AB, res = 0xAB12;

    MIUNTE_EXPECT(
        pdp11_cpu_dop_ra_instr(0000300 /* swab R0 */, x, ~x) == res,
        "swab should swap bytes correctly"
    );
    MIUNTE_EXPECT(
        !psw->nf && !psw->zf && !psw->vf && !psw->cf,
        "bit of zero result should result in {nzvc} flags = {0000}"
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
            pdp11_cpu_test_decoding_and_execution,
            pdp11_cpu_test_addressing,

            // TODO test inc/dec
            // TODO test neg/com, especially flags

            // TODO test adc/sbc, especially flags

            // TODO test asr/asl
            // TODO test ash/ashc
            // TODO test ror/rol

            pdp11_cpu_test_mov_movb,
            pdp11_cpu_test_add_sub,
            pdp11_cpu_test_cmp,
            pdp11_cpu_test_mul_div,
            pdp11_cpu_test_bit,
            pdp11_cpu_test_swab,

            // TODO test some of the branches
            // TODO test sob
        }
    );
}
