#pragma once
#include <time.h>

// clock_gettime is POSIX-only. Windows' timespec_get has sub-microsecond
// resolution but follows the wall clock, which is fine for frame timing.
static inline void getMonotonicTime(struct timespec* ts)
{
#ifdef _WIN32
    timespec_get(ts, TIME_UTC);
#else
    clock_gettime(CLOCK_MONOTONIC, ts);
#endif
}
