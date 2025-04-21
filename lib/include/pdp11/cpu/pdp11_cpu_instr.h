#ifndef PDP11_CPU_INSTR_H
#define PDP11_CPU_INSTR_H

#include <stdbool.h>
#include <stdint.h>

#include <assert.h>

typedef struct Pdp11CpuInstr {
    enum {
        PDP11_CPU_INSTR_TYPE_ILLEGAL = 0,
        PDP11_CPU_INSTR_TYPE_O,       // format 1234(6O)
        PDP11_CPU_INSTR_TYPE_OO,      // format 12(6O)(6O)
        PDP11_CPU_INSTR_TYPE_RO,      // format 123(3O)(6O)
        PDP11_CPU_INSTR_TYPE_R,       // format 12345(3O)
        PDP11_CPU_INSTR_TYPE_BRANCH,  // format 123(C)(8O)
        PDP11_CPU_INSTR_TYPE_SOB,     // sob format 077(3R)(OFF)
        PDP11_CPU_INSTR_TYPE_MISC,    // format 123456
    } type;
    union {
        struct {
            uint16_t const opcode : 10;
            uint16_t const o : 6;
        } o;
        struct {
            uint16_t const opcode : 4;
            uint16_t const o0 : 6, o1 : 6;
        } oo;
        struct {
            uint16_t const opcode : 7;
            uint16_t const r : 3, o : 6;
        } ro;
        struct {
            uint16_t const opcode : 13;
            uint16_t const r : 3;
        } r;
        struct {
            uint16_t const opcode : 7;
            bool const cond : 1;
            uint16_t const off : 8;
        } branch;
        struct {
            uint16_t const opcode : 16;
        } misc;
        struct {
            uint16_t const opcode : 7;
            uint16_t const r : 3;
            uint16_t const off : 6;
        } sob;
    } u;
} Pdp11CpuInstr;
static_assert(sizeof((Pdp11CpuInstr){0}.u) == sizeof(uint16_t));

#endif
