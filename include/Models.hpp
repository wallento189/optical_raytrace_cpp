#ifndef MODELS_HPP
#define MODELS_HPP

#include "MathUtils.hpp"

#include <string>
#include <vector>
#include <map>
#include <optional>

namespace optical {

struct Material {
    std::string name;
    double nd = 1.0;
    double nF = 1.0;
    double nC = 1.0;

    double refractiveIndex(WavelengthType wl) const {
        switch (wl) {
            case WavelengthType::D: return nd;
            case WavelengthType::F: return nF;
            case WavelengthType::C: return nC;
        }
        return nd;
    }
};

struct Surface {
    double radius = 0.0;
    double thickness = 0.0;
    std::string materialAfter = "AIR";
};

struct OpticalSystem {
    std::string systemName;
    bool objectAtInfinity = true;

    std::optional<double> objectDistance;
    std::optional<double> fieldAngleDeg;
    std::optional<double> objectHeight;

    double entrancePupilPosition = 0.0;
    double entrancePupilDiameter = 0.0;
    int stopSurface = 1;

    std::vector<double> fieldRatios;
    std::vector<double> apertureRatios;
    std::vector<WavelengthType> wavelengthTypes;

    std::map<std::string, Material> materials;
    std::vector<Surface> surfaces;
};

struct Ray {
    double L = 0.0;
    double U = 0.0;
    double height = 0.0;
    double angle = 0.0;
    WavelengthType wavelength = WavelengthType::D;
    double fieldRatio = 0.0;
    double apertureRatio = 0.0;
};

struct RayTraceResult {
    double finalL = 0.0;
    double finalU = 0.0;
    double finalHeight = 0.0;
    double finalAngle = 0.0;
    bool valid = true;
    std::string message;

    int surfacesTraced = 0;
    std::vector<double> hPerSurface;
    std::vector<double> uPerSurface;
};

struct ParaxialRayResult {
    double finalH = 0.0;
    double finalU = 0.0;
    int surfacesTraced = 0;
    std::vector<double> hPerSurface;
    std::vector<double> uPerSurface;
};

struct ParaxialResult {
    double fPrime = 0.0;
    double lPrime = 0.0;
    double lHPrime = 0.0;
    double lpPrime = 0.0;
    double y0Prime = 0.0;
    double y0707Prime = 0.0;
    double xtPrime = 0.0;
    double xsPrime = 0.0;
    double deltaXtsPrime = 0.0;
    double lFParaxial = 0.0;
    double lCParaxial = 0.0;
};

struct ResultItem {
    std::string caseName;
    int orderIndex = 0;
    std::string category;
    std::string nameZh;
    std::string name;
    double value = 0.0;
    std::string unit;
    std::string zemaxReference;
    std::string wavelength;
    std::string fieldRatio;
    std::string apertureRatio;
    std::string description;
};

struct ResultSpec {
    int orderIndex;
    std::string nameZh;
    std::string name;
    std::string category;
    std::string zemaxReference;
    std::string wavelength;
    std::string fieldRatio;
    std::string apertureRatio;
    std::string unit;
    std::string description;
};

struct FieldCurvatureResult {
    double xt = 0.0;
    double xs = 0.0;
    double astig = 0.0;
    double zTangentialFocus = 0.0;
    double zSagittalFocus = 0.0;
    double zImageSurface = 0.0;
};

struct Ray3D {
    double x = 0.0, y = 0.0, z = 0.0;
    double a = 0.0, b = 0.0, c = 1.0;
};

struct Ray3DResult {
    double x = 0.0, y = 0.0, z = 0.0;
    double a = 0.0, b = 0.0, c = 1.0;
    bool valid = true;
    std::string message;
};

struct TangentialComaResult {
    double coma = 0.0;
    double yPlus = 0.0;
    double yMinus = 0.0;
    double yChief = 0.0;
    double candidate1 = 0.0;
    double candidate2 = 0.0;
    double candidate3 = 0.0;
    double candidate4 = 0.0;
};

} // namespace optical

#endif // MODELS_HPP
