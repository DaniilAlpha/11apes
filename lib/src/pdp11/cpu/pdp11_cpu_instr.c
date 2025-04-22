#include "pdp11/cpu/pdp11_cpu_instr.h"

#include "bits.h"

Pdp11CpuInstr pdp11_cpu_instr(uint16_t const encoded) {
    switch (BITS(encoded, 0, 15)) {
    case 0000000 ... 0000006:
    case 0104000 ... 0104777:
    case 0006400 ... 0006477:
    case 0000240 ... 0000277:
        return (Pdp11CpuInstr){
            .type = PDP11_CPU_INSTR_TYPE_MISC,
            .u.misc = {.opcode = BITS(encoded, 0, 15)},
        };
    }
    switch (BITS(encoded, 3, 15)) {
    case 000020:
    case 000023:  // NOTE this is a valid instruction, just not in PDP-11/40
        return (Pdp11CpuInstr){
            .type = PDP11_CPU_INSTR_TYPE_R,
            .u.r = {.opcode = BITS(encoded, 3, 15), .r = BITS(encoded, 0, 2)},
        };
    }
    switch (BITS(encoded, 6, 15)) {
    case 00050 ... 00057:
    case 01050 ... 01057:
    case 00060 ... 00067:
    case 01060 ... 01063:
    case 00001 ... 00003:
        return (Pdp11CpuInstr){
            .type = PDP11_CPU_INSTR_TYPE_O,
            .u.o = {.opcode = BITS(encoded, 6, 15), .o = BITS(encoded, 0, 5)},
        };
    }
    switch (BITS(encoded, 9, 15)) {
    case 0070 ... 0074:
    case 0004:
        return (Pdp11CpuInstr){
            .type = PDP11_CPU_INSTR_TYPE_RO,
            .u.ro =
                {.opcode = BITS(encoded, 9, 15),
                 .r = BITS(encoded, 6, 8),
                 .o = BITS(encoded, 0, 5)},
        };
    case 0000 ... 0003:
    case 0100 ... 0103:
        return (Pdp11CpuInstr){
            .type = PDP11_CPU_INSTR_TYPE_BRANCH,
            .u.branch =
                {.opcode = BITS(encoded, 9, 15),
                 .cond = BIT(encoded, 8),
                 .off = BITS(encoded, 0, 7)},
        };
    case 0077:
        return (Pdp11CpuInstr){
            .type = PDP11_CPU_INSTR_TYPE_SOB,
            .u.sob =
                {.opcode = BITS(encoded, 9, 15),
                 .r = BITS(encoded, 6, 8),
                 .off = BITS(encoded, 0, 5)},
        };
    }
    switch (BITS(encoded, 12, 15)) {
    case 001 ... 006:
    case 011 ... 016:
        return (Pdp11CpuInstr){
            .type = PDP11_CPU_INSTR_TYPE_OO,
            .u.oo =
                {.opcode = BITS(encoded, 12, 15),
                 .o0 = BITS(encoded, 6, 11),
                 .o1 = BITS(encoded, 0, 5)},
        };
    }

    return (Pdp11CpuInstr){.type = PDP11_CPU_INSTR_TYPE_ILLEGAL};
}
