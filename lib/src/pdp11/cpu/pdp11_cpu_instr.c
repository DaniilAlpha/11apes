#include "pdp11/cpu/pdp11_cpu_instr.h"

#include "bits.h"

#define pdp11_cpu_instr_misc(...)                                              \
    (Pdp11CpuInstr) {                                                          \
        .type = PDP11_CPU_INSTR_TYPE_MISC, .u.misc = {__VA_ARGS__},            \
    }
#define pdp11_cpu_instr_r(...)                                                 \
    (Pdp11CpuInstr) { .type = PDP11_CPU_INSTR_TYPE_R, .u.r = {__VA_ARGS__}, }
#define pdp11_cpu_instr_o(...)                                                 \
    (Pdp11CpuInstr) { .type = PDP11_CPU_INSTR_TYPE_O, .u.o = {__VA_ARGS__}, }
#define pdp11_cpu_instr_ro(...)                                                \
    (Pdp11CpuInstr) { .type = PDP11_CPU_INSTR_TYPE_RO, .u.ro = {__VA_ARGS__}, }
#define pdp11_cpu_instr_oo(...)                                                \
    (Pdp11CpuInstr) { .type = PDP11_CPU_INSTR_TYPE_OO, .u.oo = {__VA_ARGS__}, }
#define pdp11_cpu_instr_branch(...)                                            \
    (Pdp11CpuInstr) {                                                          \
        .type = PDP11_CPU_INSTR_TYPE_BRANCH, .u.branch = {__VA_ARGS__},        \
    }
#define pdp11_cpu_instr_sob(...)                                               \
    (Pdp11CpuInstr) {                                                          \
        .type = PDP11_CPU_INSTR_TYPE_SOB, .u.sob = {__VA_ARGS__},              \
    }
#define pdp11_cpu_instr_jmp(...)                                               \
    (Pdp11CpuInstr) {                                                          \
        .type = PDP11_CPU_INSTR_TYPE_JMP, .u.jmp = {__VA_ARGS__},              \
    }
#define pdp11_cpu_instr_jsr(...)                                               \
    (Pdp11CpuInstr) {                                                          \
        .type = PDP11_CPU_INSTR_TYPE_JSR, .u.jsr = {__VA_ARGS__},              \
    }
#define pdp11_cpu_instr_reserved()                                             \
    (Pdp11CpuInstr) { .type = PDP11_CPU_INSTR_TYPE_RESERVED }

Pdp11CpuInstr pdp11_cpu_instr(uint16_t const encoded) {
    switch (BITS(encoded, 0, 15)) {
    case 0000000 ... 0000006:
    case 0104000 ... 0104777:
    case 0006400 ... 0006477:
    case 0000240 ... 0000277:
        return pdp11_cpu_instr_misc(.opcode = BITS(encoded, 0, 15));
    }
    switch (BITS(encoded, 3, 15)) {
    case 000020:
    case 000023:  // NOTE this is a valid instruction, just not in PDP-11/40
        return pdp11_cpu_instr_r(
                .opcode = BITS(encoded, 3, 15),
                .r = BITS(encoded, 0, 2)
        );
    }
    switch (BITS(encoded, 6, 15)) {
    case 00050 ... 00057:
    case 01050 ... 01057:
    case 00060 ... 00064:
    case 00067:
    case 01060 ... 01063:
    case 00002 ... 00003:
        return pdp11_cpu_instr_o(
                .opcode = BITS(encoded, 6, 15),
                .o = BITS(encoded, 0, 5)
        );
    case 00001:
        return pdp11_cpu_instr_jmp(
                .opcode = BITS(encoded, 6, 15),
                .o = BITS(encoded, 0, 5)
        );
    }
    switch (BITS(encoded, 9, 15)) {
    case 0070 ... 0074:
        return pdp11_cpu_instr_ro(
                .opcode = BITS(encoded, 9, 15),
                .r = BITS(encoded, 6, 8),
                .o = BITS(encoded, 0, 5)
        );
    case 0000 ... 0003:
    case 0100 ... 0103:
        if (BITS(encoded, 9, 15) == 0000 && !BIT(encoded, 8)) break;
        return pdp11_cpu_instr_branch(
                .opcode = BITS(encoded, 9, 15),
                .cond = BIT(encoded, 8),
                .off = BITS(encoded, 0, 7)
        );
    case 0077:
        return pdp11_cpu_instr_sob(
                .opcode = BITS(encoded, 9, 15),
                .r = BITS(encoded, 6, 8),
                .off = BITS(encoded, 0, 5)
        );
    case 0004:
        return pdp11_cpu_instr_jsr(
                .opcode = BITS(encoded, 9, 15),
                .r = BITS(encoded, 6, 8),
                .o = BITS(encoded, 0, 5)
        );
    }
    switch (BITS(encoded, 12, 15)) {
    case 001 ... 006:
    case 011 ... 016:
        return pdp11_cpu_instr_oo(
                .opcode = BITS(encoded, 12, 15),
                .o0 = BITS(encoded, 6, 11),
                .o1 = BITS(encoded, 0, 5)
        );
    }

    return pdp11_cpu_instr_reserved();
}
