#include <windows.h>
#include <stdint.h>

double get_current_time_seconds() {
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
    if (seconds <= 0.0) {
        return;
    }

    Sleep((DWORD)(1000.0 * seconds));
}