#include "../include/c_prototypes.h"

extern void
DieError(int errval, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    do_exit(errval);
}
