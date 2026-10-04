#pragma once

#include <cmath>

namespace glcore
{

// ========== 数学常量 ==========
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 6.28318530717958647692;
constexpr double kHalfPi = 1.57079632679489661923;
constexpr double kEpsilon = 1.0e-9;

// ========== 数值工具 ==========
constexpr float clampValue(float value, float low, float high)
{
    return value < low ? low : (value > high ? high : value);
}

constexpr double clampValue(double value, double low, double high)
{
    return value < low ? low : (value > high ? high : value);
}

constexpr float degToRad(float degrees)
{
    return static_cast<float>(static_cast<double>(degrees) * kPi / 180.0);
}

constexpr float radToDeg(float radians)
{
    return static_cast<float>(static_cast<double>(radians) * 180.0 / kPi);
}

inline bool approxZero(double value)
{
    return std::fabs(value) <= kEpsilon;
}

inline bool approxEqual(double a, double b)
{
    return std::fabs(a - b) <= kEpsilon;
}

inline bool approxEqual(float a, float b)
{
    return std::fabs(static_cast<double>(a) - static_cast<double>(b)) <= kEpsilon;
}

}  // namespace glcore
