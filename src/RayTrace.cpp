#include "RayTrace.hpp"
#include "RayGenerator.hpp"
#include "MathUtils.hpp"

#include <cmath>
#include <sstream>

namespace optical {

static double computeZCenter(double zVertexSum, double r, bool isPlane) {
    if (isPlane) return 0.0;
    return zVertexSum + r;
}

RayTraceResult traceRealRay(
    const OpticalSystem& system,
    double L1, double U1,
    WavelengthType wl
) {
    RayTraceResult result;
    result.valid = true;
    result.surfacesTraced = 0;

    if (system.surfaces.empty()) {
        result.message = "No surfaces";
        result.valid = false;
        return result;
    }

    double h = 0.0, U = U1;
    if (nearlyZero(std::sin(U1), 1e-15)) {
        h = std::fabs(L1);
    } else {
        h = -L1 * std::tan(U1);
    }

    if (!std::isfinite(h)) {
        result.message = "Non-finite initial ray height";
        result.valid = false;
        return result;
    }

    auto ait = system.materials.find("AIR");
    if (ait == system.materials.end()) {
        result.message = "AIR material not found";
        result.valid = false;
        return result;
    }

    double zRay = 0.0;
    double hRay = h;
    double zVertexSum = 0.0;
    double zLastVertex = 0.0;
    double L_last = 0.0;

    for (size_t si = 0; si < system.surfaces.size(); ++si) {
        const auto& surf = system.surfaces[si];

        double nBefore, nAfter;
        if (si == 0) {
            nBefore = ait->second.refractiveIndex(wl);
        } else {
            nBefore = system.materials.at(system.surfaces[si - 1].materialAfter).refractiveIndex(wl);
        }
        nAfter = system.materials.at(surf.materialAfter).refractiveIndex(wl);

        bool isPlane = !std::isfinite(surf.radius) || nearlyZero(surf.radius, 1e-10);
        double r = surf.radius;
        double rAbs = isPlane ? 0.0 : std::fabs(r);
        double zc = computeZCenter(zVertexSum + (si == 0 ? 0.0 : 0.0), r, isPlane);

        double z_hit = 0.0, h_hit = 0.0;
        double cosU = std::cos(U);
        double sinU = std::sin(U);

        if (isPlane) {
            double dz = zVertexSum - zRay;
            if (nearlyZero(cosU, 1e-15)) {
                result.message = "Ray parallel to plane";
                result.valid = false;
                return result;
            }
            double s = dz / cosU;
            z_hit = zVertexSum;
            h_hit = hRay + s * sinU;
        } else {
            double dz = zRay - zc;
            double A = 2.0 * (dz * cosU + hRay * sinU);
            double B = dz * dz + hRay * hRay - r * r;

            double disc = A * A - 4.0 * B;
            if (disc < -1e-12) {
                std::ostringstream oss;
                oss << "Ray misses sphere at surface " << (si + 1)
                    << " (disc=" << disc << ", zRay=" << zRay << ", hRay=" << hRay << ", zc=" << zc << ")";
                result.message = oss.str();
                result.valid = false;
                return result;
            }
            if (disc < 0.0) disc = 0.0;
            double sqrt_disc = std::sqrt(disc);

            double s1 = (-A - sqrt_disc) / 2.0;
            double s2 = (-A + sqrt_disc) / 2.0;

            double s = 0.0;
            bool s1ok = s1 > -1e-12;
            bool s2ok = s2 > -1e-12;

            if (s1ok && s2ok) {
                s = std::min(s1, s2);
            } else if (s1ok) {
                s = s1;
            } else if (s2ok) {
                s = s2;
            } else {
                std::ostringstream oss;
                oss << "No forward intersection at surface " << (si + 1)
                    << " (s1=" << s1 << ", s2=" << s2
                    << ", zRay=" << zRay << ", hRay=" << hRay << ", zc=" << zc << ")";
                result.message = oss.str();
                result.valid = false;
                return result;
            }

            z_hit = zRay + s * cosU;
            h_hit = hRay + s * sinU;
        }

        if (!std::isfinite(z_hit) || !std::isfinite(h_hit)) {
            result.message = "Non-finite intersection";
            result.valid = false;
            return result;
        }

        double sinI = 0.0, cosI = 0.0;
        if (isPlane) {
            sinI = sinU;
            cosI = cosU;
        } else {
            double nx = zc - z_hit;
            double ny = -h_hit;
            double nLen = rAbs;
            cosI = (nx * cosU + ny * sinU) / nLen;
            sinI = (-ny * cosU + nx * sinU) / nLen;
            if (cosI < 0.0) {
                cosI = -cosI;
                sinI = -sinI;
            }
        }

        double sinIp = (nBefore / nAfter) * sinI;
        if (std::fabs(sinIp) > 1.0 + 1e-10) {
            std::ostringstream oss;
            oss << "TIR at surface " << (si + 1);
            result.message = oss.str();
            result.valid = false;
            return result;
        }
        if (std::fabs(sinIp) > 1.0) {
            sinIp = std::copysign(1.0, sinIp);
        }

        double I = std::atan2(sinI, cosI);
        double Ip = std::asin(sinIp);
        double Up = U + Ip - I;

        if (nearlyZero(Up, 1e-15)) {
            L_last = std::numeric_limits<double>::infinity();
        } else {
            L_last = z_hit - h_hit / std::tan(Up);
        }

        zLastVertex = zVertexSum;
        zRay = z_hit;
        hRay = h_hit;
        U = Up;
        zVertexSum += surf.thickness;

        result.surfacesTraced++;
    }

    result.finalL = L_last - zLastVertex;
    result.finalU = U;
    result.finalHeight = hRay;
    result.finalAngle = U;

    return result;
}

RealRayFullResult traceRealRayFull(
    const OpticalSystem& system,
    double L1, double U1,
    WavelengthType wl
) {
    RealRayFullResult result;
    result.valid = true;

    if (system.surfaces.empty()) {
        result.message = "No surfaces";
        result.valid = false;
        return result;
    }

    double h = 0.0, U = U1;
    if (nearlyZero(std::sin(U1), 1e-15)) {
        h = std::fabs(L1);
    } else {
        h = -L1 * std::tan(U1);
    }

    auto ait = system.materials.find("AIR");
    if (ait == system.materials.end()) {
        result.message = "AIR not found";
        result.valid = false;
        return result;
    }

    double zRay = 0.0;
    double hRay = h;
    double zVertexSum = 0.0;
    double zLastVertex = 0.0;
    double z_hit_last = 0.0, h_hit_last = 0.0;
    double L_last = 0.0;

    for (size_t si = 0; si < system.surfaces.size(); ++si) {
        const auto& surf = system.surfaces[si];

        double nBefore, nAfter;
        if (si == 0) {
            nBefore = ait->second.refractiveIndex(wl);
        } else {
            nBefore = system.materials.at(system.surfaces[si - 1].materialAfter).refractiveIndex(wl);
        }
        nAfter = system.materials.at(surf.materialAfter).refractiveIndex(wl);

        bool isPlane = !std::isfinite(surf.radius) || nearlyZero(surf.radius, 1e-10);
        double r = surf.radius;
        double rAbs = isPlane ? 0.0 : std::fabs(r);
        double zc = computeZCenter(zVertexSum, r, isPlane);

        double z_hit = 0.0, h_hit = 0.0;
        double cosU = std::cos(U);
        double sinU = std::sin(U);

        if (isPlane) {
            double dz = zVertexSum - zRay;
            if (nearlyZero(cosU, 1e-15)) {
                result.message = "Ray parallel to plane"; result.valid = false; return result;
            }
            double s = dz / cosU;
            z_hit = zVertexSum;
            h_hit = hRay + s * sinU;
        } else {
            double dz = zRay - zc;
            double A = 2.0 * (dz * cosU + hRay * sinU);
            double B = dz * dz + hRay * hRay - r * r;
            double disc = A * A - 4.0 * B;

            if (disc < -1e-12) { result.message = "Ray misses"; result.valid = false; return result; }
            if (disc < 0) disc = 0;
            double sq = std::sqrt(disc);
            double s1 = (-A - sq) / 2.0, s2 = (-A + sq) / 2.0;

            double s = 0.0;
            bool s1ok = s1 > -1e-12, s2ok = s2 > -1e-12;
            if (s1ok && s2ok) s = std::min(s1, s2);
            else if (s1ok) s = s1;
            else if (s2ok) s = s2;
            else { result.message = "No forward intersection"; result.valid = false; return result; }

            z_hit = zRay + s * cosU;
            h_hit = hRay + s * sinU;
        }

        double sinI, cosI;
        if (isPlane) {
            sinI = sinU; cosI = cosU;
        } else {
            double nx = zc - z_hit, ny = -h_hit;
            double nLen = rAbs;
            cosI = (nx * cosU + ny * sinU) / nLen;
            sinI = (-ny * cosU + nx * sinU) / nLen;
            if (cosI < 0.0) { cosI = -cosI; sinI = -sinI; }
        }

        double sinIp = (nBefore / nAfter) * sinI;
        if (std::fabs(sinIp) > 1.0 + 1e-10) { result.message = "TIR"; result.valid = false; return result; }
        if (std::fabs(sinIp) > 1.0) sinIp = std::copysign(1.0, sinIp);

        double Ival = std::atan2(sinI, cosI);
        double Ipval = std::asin(sinIp);
        double Up = U + Ipval - Ival;

        SurfaceRayData sd;
        sd.z_hit = z_hit;
        sd.h_hit = h_hit;
        sd.I = Ival;
        sd.Ip = Ipval;
        sd.nBefore = nBefore;
        sd.nAfter = nAfter;
        sd.U = U;
        sd.Up = Up;
        result.surfaces.push_back(sd);

        if (nearlyZero(Up, 1e-15)) {
            L_last = std::numeric_limits<double>::infinity();
        } else {
            L_last = z_hit - h_hit / std::tan(Up);
        }

        z_hit_last = z_hit;
        h_hit_last = h_hit;
        zLastVertex = zVertexSum;
        zRay = z_hit;
        hRay = h_hit;
        U = Up;
        zVertexSum += surf.thickness;
    }

    result.finalL = L_last - zLastVertex;
    result.finalU = U;
    result.z_hit_last = z_hit_last;
    result.h_hit_last = h_hit_last;
    return result;
}

double intersectOpticalAxis(const RayTraceResult& result) {
    return result.finalL;
}

double heightAtImagePlane(
    const OpticalSystem&,
    const RayTraceResult& result,
    double imagePlanePosition
) {
    if (!result.valid) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (!std::isfinite(result.finalL)) {
        return result.finalHeight;
    }
    return (imagePlanePosition - result.finalL) * std::tan(result.finalU);
}

void traceCoddingtonExplicit(
    const OpticalSystem& system,
    double fieldRatio,
    WavelengthType wl,
    double lPrime,
    double tInitial, double sInitial,
    double& xtPrime,
    double& xsPrime,
    double& deltaXts
) {
    xtPrime = 0.0;
    xsPrime = 0.0;
    deltaXts = 0.0;

    if (system.surfaces.empty()) return;

    Ray chiefRay = makeInitialRay(system, fieldRatio, 0.0, wl);
    auto fullResult = traceRealRayFull(system, chiefRay.L, chiefRay.U, wl);
    if (!fullResult.valid) {
        xtPrime = std::numeric_limits<double>::quiet_NaN();
        xsPrime = std::numeric_limits<double>::quiet_NaN();
        deltaXts = std::numeric_limits<double>::quiet_NaN();
        return;
    }

    double t = tInitial, s = sInitial;
    bool tInf = !std::isfinite(t);
    bool sInf = !std::isfinite(s);

    for (size_t si = 0; si < fullResult.surfaces.size(); ++si) {
        const auto& sd = fullResult.surfaces[si];
        const auto& surf = system.surfaces[si];
        double r = surf.radius;
        double cosI = std::cos(sd.I), cosIp = std::cos(sd.Ip);

        double powerTerm = (sd.nAfter * cosIp - sd.nBefore * cosI);
        if (!std::isfinite(r) || nearlyZero(r, 1e-10)) {
            powerTerm = 0.0;
        } else {
            powerTerm /= r;
        }

        double tNextNum, tNextDen;
        if (tInf) {
            tNextNum = sd.nAfter * cosIp * cosIp;
            tNextDen = powerTerm;
        } else {
            tNextNum = sd.nAfter * cosIp * cosIp;
            tNextDen = sd.nBefore * cosI * cosI / t + powerTerm;
        }

        double sNextNum, sNextDen;
        if (sInf) {
            sNextNum = sd.nAfter;
            sNextDen = powerTerm;
        } else {
            sNextNum = sd.nAfter;
            sNextDen = sd.nBefore / s + powerTerm;
        }

        if (nearlyZero(tNextDen) || nearlyZero(sNextDen)) {
            xtPrime = std::numeric_limits<double>::quiet_NaN();
            xsPrime = std::numeric_limits<double>::quiet_NaN();
            deltaXts = std::numeric_limits<double>::quiet_NaN();
            return;
        }

        t = tNextNum / tNextDen;
        s = sNextNum / sNextDen;
        tInf = false;
        sInf = false;

        if (si < fullResult.surfaces.size() - 1) {
            const auto& sdNext = fullResult.surfaces[si + 1];
            double dx = sdNext.z_hit - sd.z_hit;
            double dy = sdNext.h_hit - sd.h_hit;
            double distAlongRay = std::sqrt(dx * dx + dy * dy);
            t -= distAlongRay;
            s -= distAlongRay;
        }
    }

    const auto& lastSd = fullResult.surfaces.back();
    double zLastGlobal = lastSd.z_hit;
    double Ulast = lastSd.Up;

    double zLastVertex = 0.0;
    for (size_t i = 0; i + 1 < system.surfaces.size(); ++i) {
        zLastVertex += system.surfaces[i].thickness;
    }

    xtPrime = t * std::cos(Ulast) + zLastGlobal - zLastVertex - lPrime;
    xsPrime = s * std::cos(Ulast) + zLastGlobal - zLastVertex - lPrime;
    deltaXts = xtPrime - xsPrime;
}

void traceCoddington(
    const OpticalSystem& system,
    double fieldRatio,
    WavelengthType wl,
    double lPrime,
    double& xtPrime,
    double& xsPrime,
    double& deltaXts
) {
    double tInit = 0.0, sInit = 0.0;
    if (system.objectAtInfinity) {
        tInit = std::numeric_limits<double>::infinity();
        sInit = std::numeric_limits<double>::infinity();
    } else if (system.objectDistance.has_value()) {
        tInit = system.objectDistance.value();
        sInit = system.objectDistance.value();
    }
    traceCoddingtonExplicit(system, fieldRatio, wl, lPrime, tInit, sInit, xtPrime, xsPrime, deltaXts);
}

} // namespace optical
