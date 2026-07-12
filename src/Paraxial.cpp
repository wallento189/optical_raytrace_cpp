#include "Paraxial.hpp"
#include "MathUtils.hpp"

#include <stdexcept>
#include <cmath>

namespace optical {

ParaxialRayResult traceParaxialRay(
    const OpticalSystem& system,
    double h1, double u1,
    WavelengthType wl
) {
    ParaxialRayResult result;
    result.hPerSurface.reserve(system.surfaces.size() + 1);
    result.uPerSurface.reserve(system.surfaces.size() + 1);

    double h = h1;
    double u = u1;

    result.hPerSurface.push_back(h);
    result.uPerSurface.push_back(u);

    std::string materialBefore = "AIR";
    auto it = system.materials.find("AIR");
    for (size_t idx = 0; idx < system.surfaces.size(); ++idx) {
        const auto& surf = system.surfaces[idx];

        if (idx > 0) {
            materialBefore = system.surfaces[idx - 1].materialAfter;
            it = system.materials.find(materialBefore);
        }
        if (it == system.materials.end()) {
            throw std::runtime_error("Material not found: " + materialBefore);
        }
        double nBefore = it->second.refractiveIndex(wl);

        auto itAfter = system.materials.find(surf.materialAfter);
        if (itAfter == system.materials.end()) {
            throw std::runtime_error("Material not found: " + surf.materialAfter);
        }
        double nAfter = itAfter->second.refractiveIndex(wl);

        double c = 0.0;
        if (std::isfinite(surf.radius) && !nearlyZero(surf.radius)) {
            c = 1.0 / surf.radius;
        }

        double uprime = (nBefore * u - h * c * (nAfter - nBefore)) / nAfter;

        u = uprime;
        h = h + surf.thickness * u;

        result.hPerSurface.push_back(h);
        result.uPerSurface.push_back(u);

        materialBefore = surf.materialAfter;
        result.surfacesTraced++;
    }

    result.finalH = h;
    result.finalU = u;

    return result;
}

SystemFirstOrder computeSystemFirstOrder(const OpticalSystem& system) {
    if (system.surfaces.empty()) {
        throw std::runtime_error("No surfaces in optical system");
    }

    double epRadius = system.entrancePupilDiameter / 2.0;
    double epPos = system.entrancePupilPosition;

    double h1 = epRadius;
    double u1 = 0.0;

    auto axialD = traceParaxialRay(system, h1, u1, WavelengthType::D);

    SystemFirstOrder fo;

    if (nearlyZero(axialD.finalU)) {
        fo.fPrime = std::numeric_limits<double>::infinity();
    } else {
        fo.fPrime = -h1 / axialD.finalU;
    }

    double lPrimeD = 0.0;
    if (!nearlyZero(axialD.finalU)) {
        lPrimeD = -axialD.finalH / axialD.finalU;
    }
    fo.lHPrime = lPrimeD - fo.fPrime;

    double omega = 0.0;
    if (system.objectAtInfinity) {
        if (system.fieldAngleDeg.has_value()) {
            omega = degToRad(system.fieldAngleDeg.value());
        }
    } else {
        if (system.objectDistance.has_value() && system.objectHeight.has_value()) {
            double Lobj = system.objectDistance.value();
            double yObj = system.objectHeight.value();
            omega = std::atan(yObj / (-Lobj));
        }
    }

    double u1chief = -omega;
    double h1chief = -u1chief * epPos;
    auto chiefD = traceParaxialRay(system, h1chief, u1chief, WavelengthType::D);

    if (!nearlyZero(chiefD.finalU)) {
        fo.lpPrime = -chiefD.finalH / chiefD.finalU;
    } else {
        fo.lpPrime = std::numeric_limits<double>::infinity();
    }

    return fo;
}

ConjugateResult computeConjugateImage(const OpticalSystem& system, const SystemFirstOrder& /*fo*/) {
    double epRadius = system.entrancePupilDiameter / 2.0;
    double epPos = system.entrancePupilPosition;

    double h1_axial = 0.0, u1_axial = 0.0;
    double fullFieldOmega = 0.0;

    if (system.objectAtInfinity) {
        h1_axial = epRadius;
        u1_axial = 0.0;
        if (!system.fieldAngleDeg.has_value())
            throw std::runtime_error("field_angle_deg required");
        fullFieldOmega = degToRad(system.fieldAngleDeg.value());
    } else {
        if (!system.objectDistance.has_value())
            throw std::runtime_error("object_distance required");
        if (!system.objectHeight.has_value())
            throw std::runtime_error("object_height required");
        double Lobj = system.objectDistance.value();
        double yObj = system.objectHeight.value();
        fullFieldOmega = yObj / std::fabs(Lobj);
        u1_axial = epRadius / (epPos - Lobj);
        h1_axial = u1_axial * (0.0 - Lobj);
    }

    auto axialD = traceParaxialRay(system, h1_axial, u1_axial, WavelengthType::D);
    auto axialF = traceParaxialRay(system, h1_axial, u1_axial, WavelengthType::F);
    auto axialC = traceParaxialRay(system, h1_axial, u1_axial, WavelengthType::C);

    auto computeLP = [](const ParaxialRayResult& r) -> double {
        if (nearlyZero(r.finalU)) return std::numeric_limits<double>::infinity();
        return -r.finalH / r.finalU;
    };

    ConjugateResult cr;
    cr.lPrimeD = computeLP(axialD);
    cr.lPrimeF = computeLP(axialF);
    cr.lPrimeC = computeLP(axialC);

    double u1_chief_full = -fullFieldOmega;
    double h1_chief_full = -u1_chief_full * epPos;
    auto chiefD = traceParaxialRay(system, h1_chief_full, u1_chief_full, WavelengthType::D);
    cr.y0Prime = chiefD.finalH + cr.lPrimeD * chiefD.finalU;

    double omega07 = 0.7 * fullFieldOmega;
    double u1_chief_07 = -omega07;
    double h1_chief_07 = -u1_chief_07 * epPos;
    auto chief07 = traceParaxialRay(system, h1_chief_07, u1_chief_07, WavelengthType::D);
    cr.y070Prime = chief07.finalH + cr.lPrimeD * chief07.finalU;

    return cr;
}

} // namespace optical
