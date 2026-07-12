#include "RayTrace3D.hpp"
#include "RayTrace.hpp"
#include "RayGenerator.hpp"
#include "MathUtils.hpp"

#include <cmath>
#include <sstream>

namespace optical {

Ray3D makeInitialRay3D(
    const OpticalSystem& system,
    double fieldRatio,
    double pupilXRatio,
    double pupilYRatio,
    WavelengthType wl
) {
    (void)wl;
    Ray3D r3d;
    double epRadius = system.entrancePupilDiameter / 2.0;
    double epPos = system.entrancePupilPosition;
    double pupilX = epRadius * pupilXRatio;
    double pupilY = epRadius * pupilYRatio;

    if (system.objectAtInfinity) {
        if (!system.fieldAngleDeg.has_value())
            throw std::runtime_error("field_angle_deg required");
        double omega = degToRad(system.fieldAngleDeg.value() * fieldRatio);
        double bx = 0.0;
        double by = std::sin(-omega);
        double bz = std::cos(omega);
        double norm = std::sqrt(bx*bx + by*by + bz*bz);
        r3d.a = bx / norm;
        r3d.b = by / norm;
        r3d.c = bz / norm;
        if (nearlyZero(r3d.c, 1e-15)) { r3d.x = pupilX; r3d.y = pupilY; r3d.z = epPos; }
        else { double t = -epPos / r3d.c; r3d.x = pupilX + r3d.a*t; r3d.y = pupilY + r3d.b*t; r3d.z = 0.0; }
    } else {
        if (!system.objectDistance.has_value() || !system.objectHeight.has_value())
            throw std::runtime_error("object_distance/height required");
        double Lobj = system.objectDistance.value();
        double yObj = system.objectHeight.value() * fieldRatio;
        double dx = pupilX - 0.0;
        double dy = pupilY - yObj;
        double dz = epPos - Lobj;
        double dLen = std::sqrt(dx*dx + dy*dy + dz*dz);
        r3d.a = dx / dLen;
        r3d.b = dy / dLen;
        r3d.c = dz / dLen;
        if (nearlyZero(r3d.c, 1e-15)) { r3d.x = pupilX; r3d.y = pupilY; r3d.z = epPos; }
        else { double t = -epPos / r3d.c; r3d.x = pupilX + r3d.a*t; r3d.y = pupilY + r3d.b*t; r3d.z = 0.0; }
    }
    return r3d;
}

Ray3DResult traceRealRay3D(
    const OpticalSystem& system,
    const Ray3D& ray,
    WavelengthType wl
) {
    Ray3DResult result;
    result.valid = true;
    double x = ray.x, y = ray.y, z = ray.z;
    double a = ray.a, b = ray.b, c = ray.c;

    if (system.surfaces.empty()) {
        result.message = "No surfaces";
        result.valid = false;
        return result;
    }

    auto ait = system.materials.find("AIR");
    if (ait == system.materials.end()) {
        result.message = "AIR not found";
        result.valid = false;
        return result;
    }

    double zVertexSum = 0.0;

    for (size_t si = 0; si < system.surfaces.size(); ++si) {
        const auto& surf = system.surfaces[si];

        double nBefore, nAfter;
        if (si == 0) nBefore = ait->second.refractiveIndex(wl);
        else nBefore = system.materials.at(system.surfaces[si - 1].materialAfter).refractiveIndex(wl);
        nAfter = system.materials.at(surf.materialAfter).refractiveIndex(wl);

        bool isPlane = !std::isfinite(surf.radius) || nearlyZero(surf.radius, 1e-10);
        double r = surf.radius;

        double ix, iy, iz;
        double nx, ny, nz;

        if (isPlane) {
            double dz = zVertexSum - z;
            if (nearlyZero(c, 1e-15)) {
                result.message = "Ray parallel to plane";
                result.valid = false;
                return result;
            }
            double s = dz / c;
            ix = x + s * a;
            iy = y + s * b;
            iz = zVertexSum;
            nx = 0.0; ny = 0.0; nz = 1.0;
        } else {
            double zc = zVertexSum + r;

            double dx0 = x - 0.0;
            double dy0 = y - 0.0;
            double dz0 = z - zc;

            double A = 2.0 * (dx0 * a + dy0 * b + dz0 * c);
            double B = dx0 * dx0 + dy0 * dy0 + dz0 * dz0 - r * r;

            double disc = A * A - 4.0 * B;
            if (disc < -1e-12) {
                result.message = "Ray misses sphere";
                result.valid = false;
                return result;
            }
            if (disc < 0.0) disc = 0.0;
            double sq = std::sqrt(disc);
            double s1 = (-A - sq) / 2.0, s2 = (-A + sq) / 2.0;
            double s = 0.0;
            bool ok1 = s1 > -1e-12, ok2 = s2 > -1e-12;
            if (ok1 && ok2) s = std::min(s1, s2);
            else if (ok1) s = s1;
            else if (ok2) s = s2;
            else { result.message = "No fwd intersection"; result.valid = false; return result; }

            ix = x + s * a;
            iy = y + s * b;
            iz = z + s * c;

            nx = ix - 0.0;
            ny = iy - 0.0;
            nz = iz - zc;
            double nLen = std::sqrt(nx * nx + ny * ny + nz * nz);
            nx /= nLen; ny /= nLen; nz /= nLen;
        }

        double cosI = -(nx * a + ny * b + nz * c);
        if (cosI < 0.0) {
            nx = -nx; ny = -ny; nz = -nz;
            cosI = -cosI;
        }

        double eta = nBefore / nAfter;
        double sinI2 = 1.0 - cosI * cosI;
        double k = 1.0 - eta * eta * sinI2;
        if (k < -1e-10) {
            result.message = "TIR";
            result.valid = false;
            return result;
        }
        if (k < 0.0) k = 0.0;
        double cosIp = std::sqrt(k);

        a = eta * a + (eta * cosI - cosIp) * nx;
        b = eta * b + (eta * cosI - cosIp) * ny;
        c = eta * c + (eta * cosI - cosIp) * nz;

        x = ix; y = iy; z = iz;
        zVertexSum += surf.thickness;
    }

    result.x = x; result.y = y; result.z = z;
    result.a = a; result.b = b; result.c = c;
    return result;
}

FieldCurvatureResult traceFieldCurvatureZemaxLike(
    const OpticalSystem& system,
    double fieldRatio,
    WavelengthType wl,
    double lPrime,
    double eps
) {
    FieldCurvatureResult fc;

    double zLastVertex = 0.0;
    for (size_t i = 0; i + 1 < system.surfaces.size(); ++i)
        zLastVertex += system.surfaces[i].thickness;
    fc.zImageSurface = zLastVertex + lPrime;

    Ray chief2D = makeInitialRay(system, fieldRatio, 0.0, wl);
    auto resChief2D = traceRealRayFull(system, chief2D.L, chief2D.U, wl);
    if (!resChief2D.valid) {
        fc.xt = fc.xs = fc.astig = std::numeric_limits<double>::quiet_NaN();
        return fc;
    }
    const auto& lastSd = resChief2D.surfaces.back();
    double zChief = lastSd.z_hit;
    double yChief = lastSd.h_hit;
    double mChief = std::tan(lastSd.Up);

    Ray tanRay = makeInitialRay(system, fieldRatio, eps, wl);
    auto resTan = traceRealRayFull(system, tanRay.L, tanRay.U, wl);
    if (!resTan.valid) {
        fc.xt = fc.xs = fc.astig = std::numeric_limits<double>::quiet_NaN();
        return fc;
    }
    const auto& tanSd = resTan.surfaces.back();
    double zTan = tanSd.z_hit;
    double yTan = tanSd.h_hit;
    double mTan = std::tan(tanSd.Up);

    double dm = mChief - mTan;
    if (nearlyZero(dm, 1e-18)) {
        fc.xt = fc.xs = fc.astig = std::numeric_limits<double>::quiet_NaN();
        return fc;
    }
    fc.zTangentialFocus = (yTan - yChief + mChief * zChief - mTan * zTan) / dm;
    fc.xt = fc.zTangentialFocus - fc.zImageSurface;

    Ray3D sagP = makeInitialRay3D(system, fieldRatio,  eps, 0.0, wl);
    auto resSP = traceRealRay3D(system, sagP, wl);
    if (!resSP.valid) {
        fc.xs = fc.astig = std::numeric_limits<double>::quiet_NaN();
        return fc;
    }
    double xP = resSP.x, zP = resSP.z, aP = resSP.a, cP = resSP.c;
    double mP = aP / cP;

    Ray3D sagM = makeInitialRay3D(system, fieldRatio, -eps, 0.0, wl);
    auto resSM = traceRealRay3D(system, sagM, wl);
    if (!resSM.valid) {
        fc.xs = fc.astig = std::numeric_limits<double>::quiet_NaN();
        return fc;
    }
    double xM = resSM.x, zM = resSM.z, aM = resSM.a, cM = resSM.c;
    double mM = aM / cM;

    double dms = mP - mM;
    if (nearlyZero(dms, 1e-18)) {
        fc.xs = fc.astig = std::numeric_limits<double>::quiet_NaN();
        return fc;
    }
    fc.zSagittalFocus = (xM - xP + mP * zP - mM * zM) / dms;
    fc.xs = fc.zSagittalFocus - fc.zImageSurface;
    fc.astig = fc.xt - fc.xs;

    return fc;
}

} // namespace optical
