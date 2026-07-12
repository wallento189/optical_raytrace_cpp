#include "MathUtils.hpp"
#include <stdexcept>

namespace optical {

std::string wavelengthToString(WavelengthType wl) {
    switch (wl) {
        case WavelengthType::D: return "d";
        case WavelengthType::F: return "F";
        case WavelengthType::C: return "C";
    }
    return "d";
}

WavelengthType wavelengthFromString(const std::string& s) {
    if (s == "d" || s == "D") return WavelengthType::D;
    if (s == "F" || s == "f") return WavelengthType::F;
    if (s == "C" || s == "c") return WavelengthType::C;
    throw std::runtime_error("Unknown wavelength type: " + s);
}

} // namespace optical
