#include "../include/c_proto.h"
#include "atomic.inc"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define QUEUE_DEFAULT_CAP  (8)
#define ASSERT_QUEUE(x)  \
  DO_WHILE(              \
    ASSERT((x));         \
    ASSERT((x)->data);   \
  )


/* ---------------------------------------------------------- Typedef's ---------------------------------------------------------- */


// typedef struct QUEUE_T *  QUEUE;


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


// struct QUEUE_T {
//   Ulong start;
//   Ulong size;
//   Ulong cap;
//   void **data;
//   void (*free_func)(void *);
// };


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


// static void queue_free_data(QUEUE q) {
//   ASSERT_QUEUE(q);
//   if (q->free_func) {
//     for (Ulong i=q->start, end=(q->start + q->size) ; i<end; ++i) {
//       q->free_func(q->data[i]);
//     }
//   }
// }


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


// QUEUE queue_create(void) {

// }
