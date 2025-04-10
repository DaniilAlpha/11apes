#ifndef CONVINIENCES_H
#define CONVINIENCES_H

#define elsizeof(ARR_) (sizeof(*ARR_))
#define lenof(ARR_)    (sizeof(ARR_) / elsizeof(ARR_))

#define forceinline __attribute__((always_inline)) inline

#define foreach(EL_, START_, END_)                                             \
  for (typeof (&*START_)(EL_) = (START_); (EL_) < (END_); (EL_)++)

#endif
