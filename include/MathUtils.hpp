#ifndef MATH_UTILS_HPP
#define MATH_UTILS_HPP

#include <string>
#include <cmath>

namespace optical {

constexpr double PI = 3.14159265358979323846;

enum class WavelengthType { D, F, C };

inline double degToRad(double deg) { return deg * PI / 180.0; }
inline double radToDeg(double rad) { return rad * 180.0 / PI; }

inline bool nearlyZero(double x, double eps = 1e-12) {
    return std::fabs(x) < eps;
}

inline bool isFiniteNumber(double x) {
    return std::isfinite(x);
}

std::string wavelengthToString(WavelengthType wl);
WavelengthType wavelengthFromString(const std::string& s);

} // namespace optical

#endif // MATH_UTILS_HPP
