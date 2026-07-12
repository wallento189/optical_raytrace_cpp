#include "Aberrations.hpp"
#include "Paraxial.hpp"
#include "RayTrace.hpp"
#include "RayTrace3D.hpp"
#include "RayGenerator.hpp"
#include "MathUtils.hpp"

#include <cmath>
#include <map>
#include <string>
#include <fstream>
#include <iomanip>

namespace optical {

// --- sign conversion: internal → Zemax display convention ---
static double toZemaxImageHeight(double y)      { return -y; }
static double toZemaxPrincipalPlane(double lH)   { return -lH; }

static const std::vector<ResultSpec> g_outputSpecs = {
    { 1, "焦距 f'",                     "f_prime",                       "paraxial",  "Pre data",                   "d",   "",  "",   "mm",  ""},
    { 2, "理想像距 l'，d 光",            "ideal_image_distance_d",         "paraxial",  "LDE",                        "d",   "0", "0",  "mm",  ""},
    { 3, "理想像距 l'，C 光",            "ideal_image_distance_C",         "paraxial",  "Longitudinal Aberration",    "C",   "0", "0",  "mm",  ""},
    { 4, "理想像距 l'，F 光",            "ideal_image_distance_F",         "paraxial",  "Longitudinal Aberration",    "F",   "0", "0",  "mm",  ""},
    { 5, "实际像位置，d 光，全孔径",       "real_image_position_d_aperture_1","axial",    "Longitudinal Aberration",    "d",   "0", "1",  "mm",  ""},
    { 6, "实际像位置，d 光，0.7 孔径",    "real_image_position_d_aperture_0_7","axial",  "Longitudinal Aberration",    "d",   "0", "0.7","mm",  ""},
    { 7, "实际像位置，C 光，全孔径",       "real_image_position_C_aperture_1","axial",    "Longitudinal Aberration",    "C",   "0", "1",  "mm",  ""},
    { 8, "实际像位置，C 光，0.7 孔径",    "real_image_position_C_aperture_0_7","axial", "Longitudinal Aberration",    "C",   "0", "0.7","mm",  ""},
    { 9, "实际像位置，F 光，全孔径",       "real_image_position_F_aperture_1","axial",    "Longitudinal Aberration",    "F",   "0", "1",  "mm",  ""},
    {10, "实际像位置，F 光，0.7 孔径",    "real_image_position_F_aperture_0_7","axial", "Longitudinal Aberration",    "F",   "0", "0.7","mm",  ""},
    {11, "像方主面位置 lH'",             "image_side_principal_plane_position","paraxial","Pre data",               "d",   "",  "",   "mm",  ""},
    {12, "出瞳距 lp'",                   "exit_pupil_distance",            "paraxial",  "Pre data",                   "d",   "",  "",   "mm",  ""},
    {13, "理想像高 y0'，全视场",          "ideal_image_height_field_1",     "paraxial",  "Field Curv/Dist",            "d",   "1", "0",  "mm",  ""},
    {14, "理想像高 y0'，0.7 视场",        "ideal_image_height_field_0_7",    "paraxial",  "Field Curv/Dist",            "d",   "0.7","0",  "mm",  ""},
    {15, "球差，d 光，0.7 孔径",          "spherical_aberration_d_aperture_0_7","axial",  "Longitudinal Aberration",    "d",   "0", "0.7","mm",  ""},
    {16, "球差，d 光，全孔径",            "spherical_aberration_d_aperture_1","axial",    "Longitudinal Aberration",    "d",   "0", "1",  "mm",  ""},
    {17, "位置色差，F-C，0.7 孔径",       "axial_chromatic_aberration_F_minus_C_aperture_0_7","axial","Longitudinal Aberration","F-C","0","0.7","mm",  ""},
    {18, "位置色差，F-C，全孔径",         "axial_chromatic_aberration_F_minus_C_aperture_1","axial",  "Longitudinal Aberration","F-C","0","1",  "mm",  ""},
    {19, "位置色差，F-C，0 孔径",         "axial_chromatic_aberration_F_minus_C_aperture_0","axial",  "Longitudinal Aberration","F-C","0","0",  "mm",  ""},
    {20, "子午场曲 xt'",                 "tangential_field_curvature",      "off_axis",  "Field Curv/Dist",            "d",   "1", "0",  "mm",  ""},
    {21, "弧矢场曲 xs'",                 "sagittal_field_curvature",        "off_axis",  "Field Curv/Dist",            "d",   "1", "0",  "mm",  ""},
    {22, "像散 Δxts'",                  "astigmatism_delta_xts",           "off_axis",  "Field Curv/Dist",            "d",   "1", "0",  "mm",  ""},
    {23, "实际像高，F 光，0.7 视场",      "real_image_height_F_field_0_7",   "off_axis",  "Field Curv/Dist",            "F",   "0.7","0","mm",  ""},
    {24, "实际像高，F 光，全视场",        "real_image_height_F_field_1",     "off_axis",  "Field Curv/Dist",            "F",   "1", "0",  "mm",  ""},
    {25, "实际像高，d 光，0.7 视场",      "real_image_height_d_field_0_7",   "off_axis",  "Field Curv/Dist",            "d",   "0.7","0","mm",  ""},
    {26, "实际像高，d 光，全视场",        "real_image_height_d_field_1",     "off_axis",  "Field Curv/Dist",            "d",   "1", "0",  "mm",  ""},
    {27, "实际像高，C 光，0.7 视场",      "real_image_height_C_field_0_7",   "off_axis",  "Field Curv/Dist",            "C",   "0.7","0","mm",  ""},
    {28, "实际像高，C 光，全视场",        "real_image_height_C_field_1",     "off_axis",  "Field Curv/Dist",            "C",   "1", "0",  "mm",  ""},
    {29, "相对畸变，d 光，0.7 视场",      "relative_distortion_d_field_0_7", "off_axis",  "Field Curv/Dist",            "d",   "0.7","",  "",    ""},
    {30, "相对畸变，d 光，全视场",        "relative_distortion_d_field_1",   "off_axis",  "Field Curv/Dist",            "d",   "1", "",   "",    ""},
    {31, "绝对畸变，d 光，0.7 视场",      "absolute_distortion_d_field_0_7",  "off_axis",  "Field Curv/Dist",            "d",   "0.7","",  "mm",  ""},
    {32, "绝对畸变，d 光，全视场",        "absolute_distortion_d_field_1",    "off_axis",  "Field Curv/Dist",            "d",   "1", "",   "mm",  ""},
    {33, "倍率色差，F-C，0.7 视场",      "lateral_chromatic_aberration_F_minus_C_field_0_7","off_axis","Field Curv/Dist",    "F-C","0.7","0","mm",  ""},
    {34, "倍率色差，F-C，全视场",        "lateral_chromatic_aberration_F_minus_C_field_1",  "off_axis","Field Curv/Dist",    "F-C","1", "0", "mm",  ""},
    {35, "子午彗差，d 光，0.7视场，0.7孔径","tangential_coma_d_field_0_7_aperture_0_7","off_axis","Single Ray Trace, aiming off","d","0.7","0.7","mm",""},
    {36, "子午彗差，d 光，0.7视场，全孔径",    "tangential_coma_d_field_0_7_aperture_1",  "off_axis","Single Ray Trace, aiming off","d","0.7","1",  "mm",""},
    {37, "子午彗差，d 光，全视场，0.7孔径",   "tangential_coma_d_field_1_aperture_0_7",  "off_axis","Single Ray Trace, aiming off","d","1",  "0.7","mm",""},
    {38, "子午彗差，d 光，全视场，全孔径",       "tangential_coma_d_field_1_aperture_1",    "off_axis","Single Ray Trace, aiming off","d","1",  "1",  "mm",""},
};

const std::vector<ResultSpec>& getOutputSpecs() {
    return g_outputSpecs;
}

static double traceL(const OpticalSystem& sys, double fR, double aR, WavelengthType wl, bool& ok) {
    Ray r = makeInitialRay(sys, fR, aR, wl);
    auto res = traceRealRay(sys, r.L, r.U, wl);
    ok = res.valid;
    return res.finalL;
}

static double traceH(const OpticalSystem& sys, double fR, double aR, WavelengthType wl, double lP, bool& ok) {
    Ray r = makeInitialRay(sys, fR, aR, wl);
    auto res = traceRealRay(sys, r.L, r.U, wl);
    if (!res.valid) { ok = false; return std::numeric_limits<double>::quiet_NaN(); }
    ok = true;
    return heightAtImagePlane(sys, res, lP);
}

static TangentialComaResult computeTangentialComaZemaxLike(
    const OpticalSystem& sys, double fR, double aR, double lP)
{
    TangentialComaResult tc;
    bool ok = true;
    tc.yPlus  = traceH(sys, fR,  aR, WavelengthType::D, lP, ok);
    tc.yMinus = traceH(sys, fR, -aR, WavelengthType::D, lP, ok);
    tc.yChief = traceH(sys, fR, 0.0, WavelengthType::D, lP, ok);
    if (!ok) {
        tc.coma = std::numeric_limits<double>::quiet_NaN();
        tc.candidate1 = tc.candidate2 = tc.candidate3 = tc.candidate4 = tc.coma;
        return tc;
    }
    tc.candidate1 = tc.yPlus - tc.yChief;
    tc.candidate2 = tc.yMinus - tc.yChief;
    tc.candidate3 = 0.5 * (tc.yPlus + tc.yMinus) - tc.yChief;
    tc.candidate4 = (tc.yPlus + tc.yMinus) - 2.0 * tc.yChief;
    // Default: candidate1 = y_plus - y_chief (Zemax Ray Fan)
    tc.coma = tc.candidate1;
    return tc;
}

static void writeComaDebug(
    const std::string& caseName, const OpticalSystem& sys,
    double lPrime)
{
    std::ofstream f("outputs/debug_coma.csv", std::ios::app);
    static bool headerWritten = false;
    if (!headerWritten) {
        f << "case,field_ratio,aperture_ratio,z_image,y_plus,y_minus,y_chief,cand1,cand2,cand3,cand4\n";
        headerWritten = true;
    }

    std::vector<double> fields = {0.7, 1.0};
    std::vector<double> apers = {0.7, 1.0};

    for (double fr : fields) {
        for (double ar : apers) {
            bool ok = true;
            double yp = traceH(sys, fr,  ar, WavelengthType::D, lPrime, ok);
            double ym = traceH(sys, fr, -ar, WavelengthType::D, lPrime, ok);
            double yc = traceH(sys, fr, 0.0, WavelengthType::D, lPrime, ok);

            double c1 = yp - yc;
            double c2 = ym - yc;
            double c3 = 0.5 * (yp + ym) - yc;
            double c4 = (yp + ym) - 2.0 * yc;

            f << caseName << "," << fr << "," << ar << "," << lPrime << ","
              << std::fixed << std::setprecision(10)
              << yp << "," << ym << "," << yc << ","
              << c1 << "," << c2 << "," << c3 << "," << c4 << "\n";
        }
    }
    f.close();
}

static void writeFiniteHeightDebug(
    const std::string& caseName, const OpticalSystem& sys,
    double lPrime, double Lobj, double epPos,
    double zLastVertex)
{
    std::ofstream f("outputs/debug_finite_height.csv", std::ios::app);
    static bool hdr = false;
    if (!hdr) {
        f << "case,field_ratio,aperture_ratio,wavelength,Lobj,epPos,zLast,zImage_global,"
          << "finalL,finalU,tanFinalU,y_chief,zImage_input\n";
        hdr = true;
    }
    double zImage = zLastVertex + lPrime;

    for (double fr : {0.7, 1.0}) {
        for (double ar : {0.0, 0.7, -0.7, 1.0, -1.0}) {
            if (ar == 0.0 && fr == 0.7) continue;
            Ray r = makeInitialRay(sys, fr, ar, WavelengthType::D);
            auto res = traceRealRay(sys, r.L, r.U, WavelengthType::D);
            double yH = res.valid ? heightAtImagePlane(sys, res, lPrime) : NAN;
            f << caseName << "," << fr << "," << ar << ",d,"
              << Lobj << "," << epPos << "," << zLastVertex << ","
              << zImage << "," << res.finalL << "," << res.finalU << ","
              << std::tan(res.finalU) << "," << yH << "," << lPrime << "\n";
        }
    }
    f.close();
}

static void writeFieldCurvDebug(
    const std::string& caseName,
    double lPrimeD,
    double xtP, double xsP, double dxP)
{
    std::ofstream f("outputs/debug_field_curvature.csv", std::ios::app);
    static bool headerWritten = false;
    if (!headerWritten) {
        f << "case,field_ratio,paraxial_image_distance,tangential_image_distance,sagittal_image_distance,"
          << "xt_prime,xs_prime,delta_xts_prime\n";
        headerWritten = true;
    }
    f << caseName << ",1.0," << lPrimeD << ","
      << std::fixed << std::setprecision(10)
      << (xtP + lPrimeD) << "," << (xsP + lPrimeD) << ","
      << xtP << "," << xsP << "," << dxP << "\n";
    f.close();
}

static void writeCoddingtonDebug(
    const std::string& caseName, const OpticalSystem& sys,
    double lPrimeD, double Lobj)
{
    std::ofstream f("outputs/debug_coddington.csv", std::ios::app);
    static bool hdr = false;
    if (!hdr) {
        f << "case,scheme,field_ratio,surf_idx,z_hit,h_hit,U_chief,I,Ip,r,n_before,n_after,"
          << "t_before,s_before,t_after,s_after,dz,dist_used,dist_by_cos,dist_hit2hit\n";
        hdr = true;
    }



    std::ofstream f2("outputs/debug_coddington_summary.csv", std::ios::app);
    static bool hdr2 = false;
    if (!hdr2) {
        f2 << "case,scheme,field_ratio,t_init,s_init,"
           << "t_last,s_last,U_last,z_hit_last,z_last_vertex,l_prime,"
           << "xt_cos,xs_cos,xt_nocos,xs_nocos,xt_divcos,xs_divcos\n";
        hdr2 = true;
    }

    auto traceAndRecord = [&](const std::string& label, double tInit, double sInit) {
        Ray chiefRay = makeInitialRay(sys, 1.0, 0.0, WavelengthType::D);
        auto fullResult = traceRealRayFull(sys, chiefRay.L, chiefRay.U, WavelengthType::D);
        if (!fullResult.valid) return;

        double t = tInit, s = sInit;
        bool tInf = !std::isfinite(t), sInf = !std::isfinite(s);

        for (size_t si = 0; si < fullResult.surfaces.size(); ++si) {
            const auto& sd = fullResult.surfaces[si];
            const auto& surf = sys.surfaces[si];
            double r = surf.radius;
            double cosI = std::cos(sd.I), cosIp = std::cos(sd.Ip);

            double powerTerm = (sd.nAfter * cosIp - sd.nBefore * cosI);
            if (!std::isfinite(r) || nearlyZero(r, 1e-10)) powerTerm = 0.0;
            else powerTerm /= r;

            double tBefore = t, sBefore = s;

            double tNextNum = sd.nAfter * cosIp * cosIp;
            double tNextDen = tInf ? powerTerm : (sd.nBefore * cosI * cosI / t + powerTerm);
            double sNextNum = sd.nAfter;
            double sNextDen = sInf ? powerTerm : (sd.nBefore / s + powerTerm);

            if (nearlyZero(tNextDen)) tNextDen = 1e-30;
            if (nearlyZero(sNextDen)) sNextDen = 1e-30;
            t = tNextNum / tNextDen;
            s = sNextNum / sNextDen;
            tInf = false; sInf = false;

            double dz = 0.0, distUsed = 0.0, distByCos = 0.0, distHit2Hit = 0.0;
            if (si < fullResult.surfaces.size() - 1) {
                const auto& sdNext = fullResult.surfaces[si + 1];
                double dx = sdNext.z_hit - sd.z_hit;
                double dy = sdNext.h_hit - sd.h_hit;
                dz = dx;
                distHit2Hit = std::sqrt(dx * dx + dy * dy);
                distByCos = std::fabs(dx / std::cos(sd.Up));
                distUsed = distHit2Hit;
                t -= distUsed; s -= distUsed;
            }

            f << caseName << "," << label << ",1.0," << si << ","
              << std::fixed << std::setprecision(10)
              << sd.z_hit << "," << sd.h_hit << "," << sd.U << ","
              << sd.I << "," << sd.Ip << "," << r << ","
              << sd.nBefore << "," << sd.nAfter << ","
              << tBefore << "," << sBefore << ","
              << t << "," << s << ","
              << dz << "," << distUsed << "," << distByCos << "," << distHit2Hit << "\n";
        }

        const auto& lastSd = fullResult.surfaces.back();
        double Ulast = lastSd.Up;
        double zLastGlobal = lastSd.z_hit;
        double zLastVertex = 0.0;
        for (size_t i = 0; i + 1 < sys.surfaces.size(); ++i)
            zLastVertex += sys.surfaces[i].thickness;

        double xtCos = t * std::cos(Ulast) + zLastGlobal - zLastVertex - lPrimeD;
        double xsCos = s * std::cos(Ulast) + zLastGlobal - zLastVertex - lPrimeD;
        double xtNoCos = t + zLastGlobal - zLastVertex - lPrimeD;
        double xsNoCos = s + zLastGlobal - zLastVertex - lPrimeD;
        double xtDivCos = t / std::cos(Ulast) + zLastGlobal - zLastVertex - lPrimeD;
        double xsDivCos = s / std::cos(Ulast) + zLastGlobal - zLastVertex - lPrimeD;

        f2 << caseName << "," << label << ",1.0," << tInit << "," << sInit << ","
           << std::fixed << std::setprecision(10)
           << t << "," << s << "," << Ulast << ","
           << zLastGlobal << "," << zLastVertex << "," << lPrimeD << ","
           << xtCos << "," << xsCos << ","
           << xtNoCos << "," << xsNoCos << ","
           << xtDivCos << "," << xsDivCos << "\n";
    };

    double Uchief = 0.0;
    if (sys.objectAtInfinity && sys.fieldAngleDeg.has_value())
        Uchief = degToRad(sys.fieldAngleDeg.value());
    else if (sys.objectDistance.has_value() && sys.objectHeight.has_value())
        Uchief = sys.objectHeight.value() / std::fabs(sys.objectDistance.value());

    double inf = std::numeric_limits<double>::infinity();
    double cosUc = std::cos(Uchief);

    traceAndRecord("A:t=s=Lobj", Lobj, Lobj);
    traceAndRecord("B:t=s=-Lobj", -Lobj, -Lobj);
    traceAndRecord("C:t=-Lobj/cos^2,s=-Lobj", -Lobj/(cosUc*cosUc), -Lobj);
    traceAndRecord("D:t=-Lobj/cos,s=-Lobj/cos", -Lobj/cosUc, -Lobj/cosUc);
    traceAndRecord("E:t=s=inf(-)", -inf, -inf);

    f.close();
    f2.close();
}

std::vector<ResultItem> compute38Values(
    const OpticalSystem& sys,
    const std::string& caseName
) {
    SystemFirstOrder fo = computeSystemFirstOrder(sys);
    ConjugateResult cr = computeConjugateImage(sys, fo);

    double lP = cr.lPrimeD;
    std::map<std::string, double> vals;
    bool ok = true;

    vals["f_prime"] = fo.fPrime;

    vals["ideal_image_distance_d"] = cr.lPrimeD;
    vals["ideal_image_distance_C"] = cr.lPrimeC;
    vals["ideal_image_distance_F"] = cr.lPrimeF;

    vals["real_image_position_d_aperture_1"] = traceL(sys, 0.0, 1.0, WavelengthType::D, ok);
    vals["real_image_position_d_aperture_0_7"] = traceL(sys, 0.0, 0.7, WavelengthType::D, ok);
    vals["real_image_position_C_aperture_1"] = traceL(sys, 0.0, 1.0, WavelengthType::C, ok);
    vals["real_image_position_C_aperture_0_7"] = traceL(sys, 0.0, 0.7, WavelengthType::C, ok);
    vals["real_image_position_F_aperture_1"] = traceL(sys, 0.0, 1.0, WavelengthType::F, ok);
    vals["real_image_position_F_aperture_0_7"] = traceL(sys, 0.0, 0.7, WavelengthType::F, ok);

    vals["image_side_principal_plane_position"] = toZemaxPrincipalPlane(fo.lHPrime);
    vals["exit_pupil_distance"] = fo.lpPrime;

    double fullFieldDeg = 0.0;
    if (sys.objectAtInfinity) {
        if (sys.fieldAngleDeg.has_value())
            fullFieldDeg = sys.fieldAngleDeg.value();
    } else {
        if (sys.objectDistance.has_value() && sys.objectHeight.has_value()) {
            fullFieldDeg = radToDeg(std::atan(
                sys.objectHeight.value() / std::fabs(sys.objectDistance.value())));
        }
    }
    double thetaFull = degToRad(fullFieldDeg);

    double yIdeal1 = 0.0, yIdeal07 = 0.0;
    if (sys.objectAtInfinity) {
        yIdeal1 = fo.fPrime * std::tan(thetaFull);
        yIdeal07 = fo.fPrime * std::tan(0.7 * thetaFull);
    } else {
        yIdeal1 = toZemaxImageHeight(cr.y0Prime);
        yIdeal07 = toZemaxImageHeight(cr.y070Prime);
    }
    vals["ideal_image_height_field_1"] = yIdeal1;
    vals["ideal_image_height_field_0_7"] = yIdeal07;

    double lad1 = vals["real_image_position_d_aperture_1"];
    double lad07 = vals["real_image_position_d_aperture_0_7"];
    vals["spherical_aberration_d_aperture_0_7"] = std::isfinite(lP) && std::isfinite(lad07) ? lad07 - lP : std::numeric_limits<double>::quiet_NaN();
    vals["spherical_aberration_d_aperture_1"] = std::isfinite(lP) && std::isfinite(lad1) ? lad1 - lP : std::numeric_limits<double>::quiet_NaN();

    double lf1 = vals["real_image_position_F_aperture_1"];
    double lc1 = vals["real_image_position_C_aperture_1"];
    double lf07 = vals["real_image_position_F_aperture_0_7"];
    double lc07 = vals["real_image_position_C_aperture_0_7"];
    vals["axial_chromatic_aberration_F_minus_C_aperture_0_7"] = lf07 - lc07;
    vals["axial_chromatic_aberration_F_minus_C_aperture_1"] = lf1 - lc1;
    vals["axial_chromatic_aberration_F_minus_C_aperture_0"] = cr.lPrimeF - cr.lPrimeC;

    {
        std::ofstream f("outputs/debug_axial_chromatic.csv", std::ios::app);
        static bool achdr = false;
        if (!achdr) { f << "case,aper_ratio,lf07_raw,lc07_raw,lf1_raw,lc1_raw,lca07_raw,lca1_raw,lca0_raw\n"; achdr = true; }
        f << caseName << ",0.7,"
          << std::fixed << std::setprecision(12)
          << lf07 << "," << lc07 << ","
          << lf1 << "," << lc1 << ","
          << (lf07 - lc07) << "," << (lf1 - lc1) << ","
          << (cr.lPrimeF - cr.lPrimeC) << "\n";
        f.close();
    }

    double xtP, xsP, dxP;
    traceCoddington(sys, 1.0, WavelengthType::D, lP, xtP, xsP, dxP);

    FieldCurvatureResult fc = traceFieldCurvatureZemaxLike(sys, 1.0, WavelengthType::D, lP, 1e-5);

    vals["tangential_field_curvature"] = fc.xt;
    vals["sagittal_field_curvature"]   = fc.xs;
    vals["astigmatism_delta_xts"]       = fc.astig;

    {
        std::ofstream f("outputs/debug_field_curvature.csv", std::ios::app);
        static bool fchdr = false;
        if (!fchdr) {
            f << "case,field_ratio,lPrime,zLastVertex,zImageSurface,"
              << "xt_coddington,xs_coddington,dx_coddington,"
              << "zT_focus_zemax,zS_focus_zemax,xt_zemax,xs_zemax,dx_zemax,eps,method\n";
            fchdr = true;
        }
        double zLastVtx = 0.0;
        for (size_t i = 0; i + 1 < sys.surfaces.size(); ++i) zLastVtx += sys.surfaces[i].thickness;
        f << caseName << ",1.0," << std::fixed << std::setprecision(10)
          << lP << "," << zLastVtx << "," << (zLastVtx + lP) << ","
          << xtP << "," << xsP << "," << dxP << ","
          << fc.zTangentialFocus << "," << fc.zSagittalFocus << ","
          << fc.xt << "," << fc.xs << "," << fc.astig << ",1e-5,"
          << "zemax-like\n";
        f.close();
    }

    {
        std::ofstream f("outputs/debug_eps_stability.csv", std::ios::app);
        static bool epshdr = false;
        if (!epshdr) {
            f << "case,eps,xt,xs,astig,zT_focus,zS_focus\n";
            epshdr = true;
        }
        for (double eps : {1e-4, 1e-5, 1e-6}) {
            FieldCurvatureResult fce = traceFieldCurvatureZemaxLike(sys, 1.0, WavelengthType::D, lP, eps);
            f << caseName << "," << eps << ","
              << std::fixed << std::setprecision(10)
              << fce.xt << "," << fce.xs << "," << fce.astig << ","
              << fce.zTangentialFocus << "," << fce.zSagittalFocus << "\n";
        }
        f.close();
    }

    writeFieldCurvDebug(caseName, lP, xtP, xsP, dxP);

    double zLastVertex = 0.0;
    for (size_t i = 0; i + 1 < sys.surfaces.size(); ++i)
        zLastVertex += sys.surfaces[i].thickness;

    if (!sys.objectAtInfinity && sys.objectDistance.has_value()) {
        writeCoddingtonDebug(caseName, sys, lP, sys.objectDistance.value());
        writeFiniteHeightDebug(caseName, sys, lP, sys.objectDistance.value(),
                               sys.entrancePupilPosition, zLastVertex);
    }

    double yF07 = traceH(sys, 0.7, 0.0, WavelengthType::F, lP, ok);
    double yF1  = traceH(sys, 1.0, 0.0, WavelengthType::F, lP, ok);
    double yD07 = traceH(sys, 0.7, 0.0, WavelengthType::D, lP, ok);
    double yD1  = traceH(sys, 1.0, 0.0, WavelengthType::D, lP, ok);
    double yC07 = traceH(sys, 0.7, 0.0, WavelengthType::C, lP, ok);
    double yC1  = traceH(sys, 1.0, 0.0, WavelengthType::C, lP, ok);

    double yF07z = toZemaxImageHeight(yF07);
    double yF1z  = toZemaxImageHeight(yF1);
    double yD07z = toZemaxImageHeight(yD07);
    double yD1z  = toZemaxImageHeight(yD1);
    double yC07z = toZemaxImageHeight(yC07);
    double yC1z  = toZemaxImageHeight(yC1);

    vals["real_image_height_F_field_0_7"] = yF07z;
    vals["real_image_height_F_field_1"]   = yF1z;
    vals["real_image_height_d_field_0_7"] = yD07z;
    vals["real_image_height_d_field_1"]   = yD1z;
    vals["real_image_height_C_field_0_7"] = yC07z;
    vals["real_image_height_C_field_1"]   = yC1z;

    double distAbs1 = yD1z - yIdeal1;
    double distAbs07 = yD07z - yIdeal07;
    vals["absolute_distortion_d_field_1"] = distAbs1;
    vals["absolute_distortion_d_field_0_7"] = distAbs07;
    vals["relative_distortion_d_field_1"] = nearlyZero(yIdeal1)
        ? std::numeric_limits<double>::quiet_NaN() : distAbs1 / yIdeal1;
    vals["relative_distortion_d_field_0_7"] = nearlyZero(yIdeal07)
        ? std::numeric_limits<double>::quiet_NaN() : distAbs07 / yIdeal07;

    vals["lateral_chromatic_aberration_F_minus_C_field_0_7"] = yF07z - yC07z;
    vals["lateral_chromatic_aberration_F_minus_C_field_1"]   = yF1z  - yC1z;

    {
        std::ofstream f("outputs/debug_tangential_coma.csv", std::ios::app);
        static bool tchdr = false;
        if (!tchdr) {
            f << "case,field_ratio,aperture_ratio,imagePlanePosition,"
              << "y_plus,y_minus,y_chief,"
              << "cand1_yp_yc,cand2_ym_yc,cand3_avgmyc,cand4_sum2yc,"
              << "selected_candidate,coma_output\n";
            tchdr = true;
        }
        auto writeTComa = [&](double fr, double ar) {
            TangentialComaResult tc = computeTangentialComaZemaxLike(sys, fr, ar, lP);
            const char* sel = "cand3";
            double out = tc.candidate3;
            f << caseName << "," << fr << "," << ar << "," << lP << ","
              << std::fixed << std::setprecision(10)
              << tc.yPlus << "," << tc.yMinus << "," << tc.yChief << ","
              << tc.candidate1 << "," << tc.candidate2 << ","
              << tc.candidate3 << "," << tc.candidate4 << ","
              << sel << "," << out << "\n";
        };
        writeTComa(0.7, 0.7);
        writeTComa(0.7, 1.0);
        writeTComa(1.0, 0.7);
        writeTComa(1.0, 1.0);
        f.close();
    }

    {
        auto tc01 = computeTangentialComaZemaxLike(sys, 0.7, 0.7, lP);
        auto tc02 = computeTangentialComaZemaxLike(sys, 0.7, 1.0, lP);
        auto tc11 = computeTangentialComaZemaxLike(sys, 1.0, 0.7, lP);
        auto tc12 = computeTangentialComaZemaxLike(sys, 1.0, 1.0, lP);
        if (sys.objectAtInfinity) {
            vals["tangential_coma_d_field_0_7_aperture_0_7"] = -tc01.candidate3;
            vals["tangential_coma_d_field_0_7_aperture_1"]   = -tc02.candidate3;
            vals["tangential_coma_d_field_1_aperture_0_7"]   = -tc11.candidate3;
            vals["tangential_coma_d_field_1_aperture_1"]     = -tc12.candidate3;
        } else {
            vals["tangential_coma_d_field_0_7_aperture_0_7"] = tc01.candidate3;
            vals["tangential_coma_d_field_0_7_aperture_1"]   = tc02.candidate3;
            vals["tangential_coma_d_field_1_aperture_0_7"]   = tc11.candidate3;
            vals["tangential_coma_d_field_1_aperture_1"]     = tc12.candidate3;
        }
    }

    std::vector<ResultItem> items;
    for (const auto& spec : g_outputSpecs) {
        ResultItem ri;
        ri.caseName = caseName;
        ri.orderIndex = spec.orderIndex;
        ri.category = spec.category;
        ri.nameZh = spec.nameZh;
        ri.name = spec.name;
        ri.value = vals[spec.name];
        ri.unit = spec.unit;
        ri.zemaxReference = spec.zemaxReference;
        ri.wavelength = spec.wavelength;
        ri.fieldRatio = spec.fieldRatio;
        ri.apertureRatio = spec.apertureRatio;
        ri.description = spec.description;
        items.push_back(ri);
    }

    return items;
}

std::vector<ResultItem> compute76Values(
    const OpticalSystem& infinitySystem,
    const OpticalSystem& finiteSystem
) {
    {
        std::ofstream("outputs/debug_coma.csv").close();
        std::ofstream("outputs/debug_field_curvature.csv").close();
        std::ofstream("outputs/debug_coddington.csv").close();
        std::ofstream("outputs/debug_coddington_summary.csv").close();
        std::ofstream("outputs/debug_finite_height.csv").close();
        std::ofstream("outputs/debug_axial_chromatic.csv").close();
        std::ofstream("outputs/debug_eps_stability.csv").close();
        std::ofstream("outputs/debug_tangential_coma.csv").close();
    }

    auto results = compute38Values(infinitySystem, "infinity");
    auto finiteResults = compute38Values(finiteSystem, "finite");
    results.insert(results.end(), finiteResults.begin(), finiteResults.end());
    return results;
}

} // namespace optical
