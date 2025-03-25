#ifndef PDP11_OP_H
#define PDP11_OP_H

#include "conviniences.h"
#include "pdp11/pdp11.h"

void pdp11_op_exec(Pdp11 *const self, uint16_t const instr);

#endif
