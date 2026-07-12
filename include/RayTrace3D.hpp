#ifndef RAY_TRACE_3D_HPP
#define RAY_TRACE_3D_HPP

#include "Models.hpp"

namespace optical {

Ray3D makeInitialRay3D(
    const OpticalSystem& system,
    double fieldRatio,
    double pupilXRatio,
    double pupilYRatio,
    WavelengthType wl
);

Ray3DResult traceRealRay3D(
    const OpticalSystem& system,
    const Ray3D& ray,
    WavelengthType wl
);

FieldCurvatureResult traceFieldCurvatureZemaxLike(
    const OpticalSystem& system,
    double fieldRatio,
    WavelengthType wl,
    double lPrime,
    double eps
);

} // namespace optical

#endif // RAY_TRACE_3D_HPP
