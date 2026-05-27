#include "timer.h"
#include <windows.h>

Timer::Timer() : start_(now_ms()) {}

void Timer::reset() { start_ = now_ms(); }

double Timer::elapsed_ms() const { return now_ms() - start_; }

double Timer::elapsed_us() const { return elapsed_ms() * 1000.0; }

double Timer::now_ms() const {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return static_cast<double>(counter.QuadPart) * 1000.0 / freq.QuadPart;
}

void Timer::sleep_ms(double ms) const {
    if (ms <= 0) return;
    DWORD ms_int = static_cast<DWORD>(ms);
    HANDLE timer = CreateWaitableTimer(nullptr, FALSE, nullptr);
    if (timer) {
        LARGE_INTEGER due;
        due.QuadPart = -static_cast<LONGLONG>(ms * 10000.0);
        SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE);
        WaitForSingleObject(timer, INFINITE);
        CloseHandle(timer);
    }
}

void Timer::sleep_until_ms(double target_ms) const {
    double remaining = target_ms - elapsed_ms();
    if (remaining > 0.5) {
        sleep_ms(remaining - 0.3);
        while (elapsed_ms() < target_ms) {
            Sleep(0);
        }
    }
}
