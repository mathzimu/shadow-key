#include "timer.h"
#include <chrono>
#include <thread>

#if defined(_WIN32)
    #include <windows.h>
#endif

Timer::Timer() noexcept : start_(now_ms()) {}

void Timer::reset() noexcept { start_ = now_ms(); }

double Timer::elapsed_ms() const noexcept { return now_ms() - start_; }

double Timer::elapsed_us() const noexcept { return elapsed_ms() * 1000.0; }

double Timer::now_ms() noexcept {
#if defined(_WIN32)
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return static_cast<double>(counter.QuadPart) * 1000.0 / freq.QuadPart;
#else
    using namespace std::chrono;
    return duration_cast<duration<double, std::milli>>(
               high_resolution_clock::now().time_since_epoch()).count();
#endif
}

void Timer::sleep_ms(double ms) noexcept {
    if (ms <= 0) return;
    std::this_thread::sleep_for(
        std::chrono::milliseconds(static_cast<long long>(ms)));
}

void Timer::sleep_until_ms(double target_ms) const noexcept {
    double remaining = target_ms - elapsed_ms();
    if (remaining > 0.5) {
        sleep_ms(remaining - 0.3);
        while (elapsed_ms() < target_ms) {
            std::this_thread::sleep_for(std::chrono::microseconds(300));
        }
    }
}
