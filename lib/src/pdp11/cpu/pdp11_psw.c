#include "pdp11/cpu/pdp11_psw.h"

Result pdp11_psw_init(Pdp11Psw *const self) {
    if (pthread_cond_init(&self->_priority_changed, NULL) != 0)
        return UnknownErr;

    self->_priority = 0;
    self->flags = (Pdp11PswFlags){0};

    return Ok;
}
void pdp11_psw_uninit(Pdp11Psw *const self) {
    pthread_cond_destroy(&self->_priority_changed);
}

void pdp11_psw_wait_for_sufficient_priority(
    Pdp11Psw *const self,
    uint8_t const priority,
    pthread_mutex_t *const lock
) {
    while (priority <= self->_priority)
        pthread_cond_wait(&self->_priority_changed, lock);
}
