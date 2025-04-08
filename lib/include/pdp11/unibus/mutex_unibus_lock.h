#ifndef MUTEX_UNIBUS_LOCK_H
#define MUTEX_UNIBUS_LOCK_H

#include <pthread.h>

#include "pdp11/unibus/unibus_lock.h"

UnibusLock pthread_mutex_ww_unibus_lock(pthread_mutex_t *const self);

#endif
