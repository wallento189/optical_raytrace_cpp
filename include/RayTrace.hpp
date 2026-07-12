#ifndef RAY_TRACE_HPP
#define RAY_TRACE_HPP

#include "Models.hpp"

namespace optical {

struct SurfaceRayData {
    double z_hit;
    double h_hit;
    double I;
    double Ip;
    double nBefore;
    double nAfter;
    double U;
    double Up;
};

struct RealRayFullResult {
    double finalL;
    double finalU;
    double z_hit_last;
    double h_hit_last;
    bool valid;
    std::string message;
    std::vector<SurfaceRayData> surfaces;
};

RayTraceResult traceRealRay(
    const OpticalSystem& system,
    double L1, double U1,
    WavelengthType wl
);

RealRayFullResult traceRealRayFull(
    const OpticalSystem& system,
    double L1, double U1,
    WavelengthType wl
);

void traceCoddington(
    const OpticalSystem& system,
    double fieldRatio,
    WavelengthType wl,
    double lPrime,
    double& xtPrime,
    double& xsPrime,
    double& deltaXts
);

void traceCoddingtonExplicit(
    const OpticalSystem& system,
    double fieldRatio,
    WavelengthType wl,
    double lPrime,
    double tInitial, double sInitial,
    double& xtPrime,
    double& xsPrime,
    double& deltaXts
);

double intersectOpticalAxis(const RayTraceResult& result);

double heightAtImagePlane(
    const OpticalSystem& system,
    const RayTraceResult& result,
    double imagePlanePosition
);

} // namespace optical

#endif // RAY_TRACE_HPP
