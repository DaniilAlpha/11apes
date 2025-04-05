#ifndef UNIBUS_LOCK_H
#define UNIBUS_LOCK_H

#include <woodi.h>

#define UNIBUS_LOCK_INTERFACE(Self)                                            \
    {                                                                          \
        void (*lock)(Self *const self);                                        \
        void (*unlock)(Self *const self);                                      \
    }
WRAPPER(UnibusLock, UNIBUS_LOCK_INTERFACE);

static inline void unibus_lock_lock(UnibusLock *const self) {
    WRAPPER_CALL(lock, self);
}
static inline void unibus_lock_unlock(UnibusLock *const self) {
    WRAPPER_CALL(unlock, self);
}

#endif
