#ifndef SCOPEDTIMER_H
#define SCOPEDTIMER_H

#include <chrono>
#include <string>
#include <iostream>

/**
 * Utility class for a simple time measurement. Prints time since object contruction until destruction.
 * Usage:
 * {
 *    ScopedTimer t{"parsing"};
 *    <parsing>
 * }
 */
template<typename Clock = std::chrono::steady_clock,
         typename Duration = std::chrono::milliseconds>
class ScopedTimer
{
private:
    std::string m_message;
    std::chrono::time_point<Clock> start;
    static constexpr const char* unitStr {std::is_same_v<Duration, std::chrono::hours> ? "h" :
                                          std::is_same_v<Duration, std::chrono::minutes> ? "min" :
                                          std::is_same_v<Duration, std::chrono::seconds> ? "s" :
                                          std::is_same_v<Duration, std::chrono::milliseconds> ? "ms" :
                                          std::is_same_v<Duration, std::chrono::microseconds> ? "us" :
                                          std::is_same_v<Duration, std::chrono::nanoseconds> ? "ns" : ""};
public:
    ScopedTimer(std::string message) noexcept
        : m_message(std::move(message)),
          start(Clock::now())
    {}

    ~ScopedTimer() noexcept
    {
        auto elapsed =
            std::chrono::duration_cast<Duration>(Clock::now() - start);
        std::cout << m_message << ": " << elapsed.count() << unitStr << std::endl;;
    }
};

#endif // SCOPEDTIMER_H
