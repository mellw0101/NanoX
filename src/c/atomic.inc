#include "../include/c_proto.h"


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- atomic_fetch_add ----------------------------- */

static __always_inline Ulong atomic_fetch_add(Ulong *p, Ulong inc) {
  Ulong v = inc;
  __asm__ __volatile__(
    "lock xaddq %0, %1"
    : "+r"(v), "+m"(*p)
    :
    : "memory", "cc"
  );
  return v;
}

/* ----------------------------- atomic_sub ----------------------------- */

static __always_inline void atomic_sub(Ulong *p, Ulong sub) {
  __asm__ __volatile__(
    "lock subq %1, %0"
    : "+m"(*p)
    : "r"(sub)
    : "memory", "cc"
  );
}

/* ----------------------------- atomic_get_array_ptr_inc_idx ----------------------------- */

static __always_inline void *atomic_get_array_ptr_inc_idx(void *const *arr, Ulong *idx, Ulong max) {
  Ulong i = atomic_fetch_add(idx, 1);
  if (i >= max) {
    return NULL;
  }
  return arr[i];
}
