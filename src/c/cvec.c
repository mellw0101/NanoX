/** @file cvec.c

  @author  Melwin Svensson.
  @date    17-2-2025.

 */
#include "../include/c_proto.h"

struct CVec {
  /* Cap value for new 'CVec' structures. */
  #define CVEC_INITIAL_CAP  (10)
  
  /* Perform some action under protection of the vector mutex. */
  #define CVEC_MUTEX_ACTION(action)   \
  MACRO_DO_WHILE(                   \
    ASSERT(v);                      \
    under_mutex(&v->mutex,          \
    ASSERT(v->cap);               \
    ASSERT(v->data);              \
    MACRO_DO_WHILE(action);       \
    );                              \
  )

  int len;           /* Current number of elements in the vector. */
  int cap;           /* Allocated size in number of elements of vector. */
  void **data;       /* Generic ptr to hold any type of ptr's. */
  FreeFuncPtr free;  /* Ptr to funtion used for deallocation this way we can enforse thread-safety. */
  mutex_t mutex;     /* Mutex for fully threaded operations. */
};

/* Create a new blank allocated CVec structure. */
CVec *cvec_create(void) {
  CVec *v = xmalloc(sizeof(*v));
  v->len  = 0;
  v->cap  = CVEC_INITIAL_CAP;
  v->data = xmalloc(v->cap * sizeof(void *));
  v->free = NULL;
  init_mutex(&v->mutex, NULL);
  return v;
}

/* Free a CVec structure. */
void cvec_free(CVec *const v) {
  CVEC_MUTEX_ACTION(
    /* Only iterate when the free function is valid. */
    if (v->free) {
      /* Free all elements using the chosen free function. */
      for (int i=0; i<v->len; ++i) {
      v->free(v->data[i]);
      }
    }
    free(v->data);
  );
  destroy_mutex(&v->mutex);
  free(v);
}

/* Set the element free function.  Note that free() is the default one and vill be used if none is chosen, pass NULL to not free elements. */
void cvec_setfree(CVec *const v, FreeFuncPtr free) {
  CVEC_MUTEX_ACTION(
    v->free = free;
  );
}

/* Add 'item' to the back of v. */
void cvec_push(CVec *const v, void *const item) {
  CVEC_MUTEX_ACTION(
  ENSURE_PTR_ARRAY_SIZE(v->data, v->cap, v->len);
    v->data[v->len++] = item;
  );
}

/* Trim the internal data ptr of v, to save memory. */
void cvec_trim(CVec *const v) {
  CVEC_MUTEX_ACTION(
    TRIM_PTR_ARRAY(v->data, v->cap, v->len);
  );
}

/* Get the item at index inside v. */
void *cvec_get(CVec *const v, int index) {
  void *ret;
  CVEC_MUTEX_ACTION(
    ASSERT(index < v->len);
    ret = v->data[index];
  );
  return ret;
}
