#include "RayGenerator.hpp"
#include "MathUtils.hpp"

#include <stdexcept>
#include <cmath>

namespace optical {

Ray makeInitialRay(
    const OpticalSystem& system,
    double fieldRatio,
    double apertureRatio,
    WavelengthType wavelength
) {
    Ray ray;
    ray.wavelength = wavelength;
    ray.fieldRatio = fieldRatio;
    ray.apertureRatio = apertureRatio;

    double epRadius = system.entrancePupilDiameter / 2.0;
    double epPos = system.entrancePupilPosition;
    double pupilY = epRadius * apertureRatio;

    if (system.objectAtInfinity) {
        if (!system.fieldAngleDeg.has_value()) {
            throw std::runtime_error("field_angle_deg required for infinite object");
        }
        double omega = -degToRad(system.fieldAngleDeg.value() * fieldRatio);

        double h1 = pupilY - std::tan(omega) * epPos;
        double U1 = omega;

        ray.height = h1;
        ray.angle = U1;

        if (nearlyZero(std::sin(U1), 1e-15)) {
            ray.L = std::fabs(h1);
        } else {
            ray.L = -h1 / std::tan(U1);
        }
        ray.U = U1;
    } else {
        if (!system.objectDistance.has_value()) {
            throw std::runtime_error("object_distance required for finite object");
        }
        if (!system.objectHeight.has_value()) {
            throw std::runtime_error("object_height required for finite object");
        }
        double Lobj = system.objectDistance.value();
        double yObj = system.objectHeight.value() * fieldRatio;

        double slope = (pupilY - yObj) / (epPos - Lobj);
        double U1 = std::atan(slope);
        double h1 = yObj + std::tan(U1) * (0.0 - Lobj);

        ray.height = h1;
        ray.angle = U1;

        if (nearlyZero(std::sin(U1), 1e-15)) {
            ray.L = std::fabs(h1);
        } else {
            ray.L = -h1 / std::tan(U1);
        }
        ray.U = U1;
    }

    return ray;
}

std::vector<Ray> generateRequiredRays(const OpticalSystem& system) {
    std::vector<Ray> rays;

    std::vector<double> apRatios = {1.0, 0.707};
    std::vector<double> fieldRats = {1.0, 0.707, 0.0};

    for (auto wl : system.wavelengthTypes) {
        for (double fr : fieldRats) {
            bool isAxial = nearlyZero(fr);

            if (isAxial) {
                for (double ar : apRatios) {
                    if (ar > 0) {
                        rays.push_back(makeInitialRay(system, fr, ar, wl));
                    }
                }
            } else {
                rays.push_back(makeInitialRay(system, fr, 0.0, wl));
                if (wl == WavelengthType::D) {
                    rays.push_back(makeInitialRay(system, fr, 1.0, wl));
                    rays.push_back(makeInitialRay(system, fr, -1.0, wl));
                    rays.push_back(makeInitialRay(system, fr, 0.707, wl));
                    rays.push_back(makeInitialRay(system, fr, -0.707, wl));
                }
            }
        }
    }

    return rays;
}

} // namespace optical
