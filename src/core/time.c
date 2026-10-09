#include <stdint.h>

#if defined(_WIN32)

#include <windows.h>

double get_current_time_seconds(void) {
    static LARGE_INTEGER frequency;
    static int initialized = 0;
    LARGE_INTEGER counter;

    if (!initialized) {
        QueryPerformanceFrequency(&frequency);
        initialized = 1;
    }

    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
}

void sleep_s(double seconds) {
    if (seconds <= 0.0) return;
    Sleep((DWORD)(1000.0 * seconds));
}

#elif defined(__APPLE__)

#include <mach/mach_time.h>
#include <time.h>

double get_current_time_seconds(void) {
    static mach_timebase_info_data_t tb;
    static int initialized = 0;

    if (!initialized) {
        mach_timebase_info(&tb);
        initialized = 1;
    }

    uint64_t t = mach_absolute_time();
    return (double)t * (double)tb.numer / (double)tb.denom / 1e9;
}

void sleep_s(double seconds) {
    if (seconds <= 0.0) return;

    struct timespec ts;
    ts.tv_sec  = (time_t)seconds;
    ts.tv_nsec = (long)((seconds - (double)ts.tv_sec) * 1e9);
    nanosleep(&ts, NULL);
}

#else
#error "time.c: plataforma no soportada"
#endif