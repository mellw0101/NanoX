/** @file nassert.h

  @author  Melwin Svensson.
  @date    27-1-2025.

  This file is part of NanoX.

  Declares some assert functions and functionality.

 */
#pragma once

#define ASSERT_DO_WHILE(...)  do {__VA_ARGS__} while(0)

/* Our assert casts. */
#define ASSERT_NO_CAST                  ((void)0)
#define ASSERT_DIE_CAST(expr)           die("%s: LINE: %lu: FILE: %s: Assertion failed: %s\n", __func__, __LINE__, __FILE__, #expr)
#define ASSERT_DIE_MSG_CAST(expr, msg)  die("%s: LINE: %lu: FILE: %s: Assertion failed: %s: %s\n", __func__, __LINE__, __FILE__, #expr, (msg))

#define DO_ITER_CIRCULAR_LIST(type, start, name, action) \
  ASSERT_DO_WHILE(                                       \
    type name = start;                                   \
    do {                                                 \
      ASSERT_DO_WHILE(action);                           \
      name = name->next;                                 \
    } while (name != start);                             \
  )

/* Our assert, that dies like we want, safely. */
#define DO_ASSERT(expr)         \
  ASSERT_DO_WHILE(              \
    (expr)                      \
      ? ASSERT_NO_CAST          \
      : ASSERT_DIE_CAST(expr);  \
  )

/* Same thing as our assert, just including a msg. */
#define DO_ASSERT_MSG(expr, msg)         \
  ASSERT_DO_WHILE(                       \
    (expr)                               \
      ? ASSERT_NO_CAST                   \
      : ASSERT_DIE_MSG_CAST(expr, msg);  \
  )

#define DO_ASSERT_CIRCULAR_LIST_PTR(list)  \
  ASSERT_DO_WHILE(                     \
    DO_ASSERT(list);                   \
    DO_ASSERT(list->next);             \
    DO_ASSERT(list->prev);             \
  )

#define DO_ASSERT_WHOLE_CIRCULAR_LIST(type, start) \
  DO_ITER_CIRCULAR_LIST(type, start, ptr, \
    DO_ASSERT(ptr); \
  )

#ifdef ASSERT_DEBUG
# define ASSERT(expr)                             DO_ASSERT(expr)
# define ASSERT_MSG(expr, msg)                    DO_ASSERT_MSG(expr, msg)
# define ASSERT_CIRCULAR_LIST_PTR(list)           DO_ASSERT_CIRCULAR_LIST_PTR(list)
# define ASSERT_WHOLE_CIRCULAR_LIST(type, start)  DO_ASSERT_WHOLE_CIRCULAR_LIST(type, start)
#else
# define ASSERT(expr)                             ASSERT_NO_CAST
# define ASSERT_MSG(expr, msg)                    ASSERT_NO_CAST
# define ASSERT_CIRCULAR_LIST_PTR(list)           ASSERT_NO_CAST
# define ASSERT_WHOLE_CIRCULAR_LIST(type, start)  ASSERT_NO_CAST
#endif

/* For things that we should always assert, for instance
 * fully dynamic things that can fail based on current env. */
#define ALWAYS_ASSERT(expr)                             DO_ASSERT(expr)
#define ALWAYS_ASSERT_MSG(expr, msg)                    DO_ASSERT_MSG(expr, msg)
#define ALWAYS_ASSERT_CIRCULAR_LIST_PTR(list)           DO_ASSERT_CIRCULAR_LIST_PTR(list)
#define ALWAYS_ASSERT_WHOLE_CIRCULAR_LIST(type, start)  DO_ASSERT_WHOLE_CIRCULAR_LIST(type, start)
