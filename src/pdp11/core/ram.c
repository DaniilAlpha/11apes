#include "pdp11/core/ram.h"

#include <stddef.h>
#include <stdlib.h>

#include "conviniences.h"

Result ram_init(Ram *const self) {
    self->byte = NULL;

    uint8_t *const data = malloc(RAM_SIZE * elsizeof(data));
    if (!data) return OutOfMemErr;

    self->byte = data;

    return Ok;
}

void ram_uninit(Ram *const self) { free(self->byte); }
