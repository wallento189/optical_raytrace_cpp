#ifndef PARAXIAL_HPP
#define PARAXIAL_HPP

#include "Models.hpp"

namespace optical {

struct SystemFirstOrder {
    double fPrime = 0.0;
    double lHPrime = 0.0;
    double lpPrime = 0.0;
};

struct ConjugateResult {
    double lPrimeD = 0.0;
    double lPrimeF = 0.0;
    double lPrimeC = 0.0;
    double y0Prime = 0.0;
    double y070Prime = 0.0;
};

ParaxialRayResult traceParaxialRay(
    const OpticalSystem& system,
    double h1, double u1,
    WavelengthType wl
);

SystemFirstOrder computeSystemFirstOrder(const OpticalSystem& system);
ConjugateResult computeConjugateImage(const OpticalSystem& system, const SystemFirstOrder& fo);

} // namespace optical

#endif // PARAXIAL_HPP
