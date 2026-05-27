#pragma once
#include <cstdint>

class Timer {
public:
    Timer();
    void reset();
    double elapsed_ms() const;
    double elapsed_us() const;
    void sleep_ms(double ms) const;
    void sleep_until_ms(double target_ms) const;

private:
    double start_;
    double now_ms() const;
};
