#ifndef UTIL_H
#define UTIL_H

#include <limits>
#include <cmath>
#include <optional>
#include <string>
#include <type_traits>

/**
 * Parse string to double. This is 2 times faster than the std library implementation
 * with a small error, which is totally fine for using the result for drawing purposes.
 */
template<typename iterator>
constexpr std::optional<double> str_to_double_noexp(const iterator start, const iterator end) noexcept
{
    iterator p = start;
    double result = 0.0;

    for (;; p++)
    {
        if (p == end)
            return result;

        auto digit = unsigned(*p) - '0';
        if (digit > 9)
            break;

        result = result * 10.0 + digit;
    }

    if (result == std::numeric_limits<double>::infinity())
        return std::nullopt;

    if (*p == '.')
    {
        p++;

        double factor = 0.1;
        for (; p != end; p++)
        {
            auto digit = unsigned(*p) - '0';
            // assume it is a valid digit
            result += digit * factor;
            factor *= 0.1;
        }
    }

    return result;
}

/**
 * Parse S840D-style floating point literal with EX instead of E for an exponent.
 */
template<typename iterator>
constexpr std::optional<double> str_to_double_s840d_exp(iterator start, iterator end) noexcept
{
    iterator p = start;
    double result = 0.0;

    for (;; p++)
    {
        if (p == end)
            return result;

        auto digit = unsigned(*p) - '0';
        if (digit > 9)
            break;

        result = result * 10.0 + digit;
    }

    if (result == std::numeric_limits<double>::infinity())
        return std::nullopt;

    if (*p == '.')
    {
        p++;

        double factor = 0.1;
        for (;; p++)
        {
            if (p == end)
                return result;

            auto digit = unsigned(*p) - '0';
            if (digit > 9)
                break;

            result += digit * factor;
            factor *= 0.1;
        }
    }

    if (p == end)
        return result;

    if (*p == 'E' || *p == 'e')
    {
        p += 2;//skip 'EX'

        bool expMinus = false;
        switch (*p)
        {
        case '-':
            expMinus = true;
            [[fallthrough]];
        case '+':
            p++;
        }

        int exp = *p - '0';
        p++;
        for (; p != end; p++)
        {
            auto digit = unsigned(*p) - '0';
            // assume it is a valid digit
            exp = exp * 10 + digit;
            if (exp > std::numeric_limits<double>::max_exponent10)
                return std::nullopt;
        }

        return result * std::pow(10, expMinus ? -exp : exp);
    }

    // if we are here then "end" iterator is wrong
    return result;
}

template<typename String>
void to_upper(String& s) noexcept
{
    for (auto& c : s)
        c = toupper(c);
}

template<typename String>
void to_lower(String& s) noexcept
{
    for (auto& c : s)
        c = tolower(c);
}

template<typename String>
typename std::decay_t<String> to_upper_copy(String& s) noexcept
{
    typename std::decay_t<String> copy {s};
    for (auto& c : copy)
        c = toupper(c);
    return copy;
}

template<typename String>
typename std::decay_t<String> to_lower_copy(String& s) noexcept
{
    typename std::decay_t<String> copy {s};
    for (auto& c : copy)
        c = tolower(c);
    return copy;
}

#endif // UTIL_H
