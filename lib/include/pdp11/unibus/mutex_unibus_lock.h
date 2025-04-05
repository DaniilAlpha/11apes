#ifndef MUTEX_UNIBUS_LOCK_H
#define MUTEX_UNIBUS_LOCK_H

#include <pthread.h>

#include "pdp11/unibus/unibus_lock.h"

void mutex_unibus_lock_lock(pthread_mutex_t *const self) {
    pthread_mutex_lock(self);
}
void mutex_unibus_lock_unlock(pthread_mutex_t *const self) {
    pthread_mutex_unlock(self);
}

UnibusLock pthread_mutex_ww_unibus_lock(pthread_mutex_t *const self) WRAP_BODY(
    UnibusLock,
    UNIBUS_LOCK_INTERFACE(pthread_mutex_t),
    {
        .lock = mutex_unibus_lock_lock,
        .unlock = mutex_unibus_lock_unlock,
    }
);

#endif
