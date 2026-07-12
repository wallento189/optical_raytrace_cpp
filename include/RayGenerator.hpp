#ifndef RAY_GENERATOR_HPP
#define RAY_GENERATOR_HPP

#include "Models.hpp"

namespace optical {

Ray makeInitialRay(
    const OpticalSystem& system,
    double fieldRatio,
    double apertureRatio,
    WavelengthType wavelength
);

std::vector<Ray> generateRequiredRays(const OpticalSystem& system);

} // namespace optical

#endif // RAY_GENERATOR_HPP
