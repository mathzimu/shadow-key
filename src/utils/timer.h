#pragma once
#include <cstdint>
#include <chrono>
#include <thread>

/// High-precision timer using QueryPerformanceCounter.
class Timer {
public:
    Timer() noexcept;

    /// Reset the timer to zero.
    void reset() noexcept;

    /// Milliseconds elapsed since construction or last reset.
    [[nodiscard]] double elapsed_ms() const noexcept;

    /// Microseconds elapsed since construction or last reset.
    [[nodiscard]] double elapsed_us() const noexcept;

    /// Block for at least @p ms milliseconds.
    static void sleep_ms(double ms) noexcept;

    /// Spin until @p target_ms milliseconds have elapsed since the last reset.
    void sleep_until_ms(double target_ms) const noexcept;

private:
    double start_;
    [[nodiscard]] static double now_ms() noexcept;
};
