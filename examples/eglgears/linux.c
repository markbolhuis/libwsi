#define _POSIX_C_SOURCE 201112L
#include <stdint.h>
#include <time.h>

int64_t
get_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000 + ts.tv_nsec;
}
