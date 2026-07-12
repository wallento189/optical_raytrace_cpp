#include "IOUtils.hpp"
#include "MathUtils.hpp"

#include "json.hpp"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <iostream>
#include <algorithm>

namespace optical {

using json = nlohmann::json;

static double parseRadiusValue(const json& jVal) {
    if (jVal.is_number()) {
        return jVal.get<double>();
    }
    if (jVal.is_string()) {
        std::string s = jVal.get<std::string>();
        if (s == "inf" || s == "INF" || s == "Infinity" || s == "infinity") {
            return std::numeric_limits<double>::infinity();
        }
        try {
            return std::stod(s);
        } catch (...) {
            throw std::runtime_error("Cannot parse radius value: " + s);
        }
    }
    throw std::runtime_error("Radius value must be number or string");
}

static double parseNumericValue(const json& jVal, const std::string& fieldName) {
    if (jVal.is_number()) {
        return jVal.get<double>();
    }
    if (jVal.is_string()) {
        std::string s = jVal.get<std::string>();
        if (s == "inf" || s == "INF" || s == "Infinity" || s == "infinity") {
            return std::numeric_limits<double>::infinity();
        }
        if (s == "nan" || s == "NaN" || s == "NAN") {
            return std::numeric_limits<double>::quiet_NaN();
        }
        throw std::runtime_error("Field '" + fieldName + "' has a string value that is not a valid number placeholder: " + s +
                                 ". Please fill in the actual numeric value.");
    }
    throw std::runtime_error("Field '" + fieldName + "' must be a number");
}

OpticalSystem loadSystemFromJson(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }

    json j;
    try {
        f >> j;
    } catch (const json::parse_error& e) {
        throw std::runtime_error("JSON parse error in " + path + ": " + std::string(e.what()));
    }

    OpticalSystem sys;

    sys.systemName = j.value("system_name", "unnamed");

    sys.objectAtInfinity = j.value("object_at_infinity", true);

    if (j.contains("object_distance") && !j["object_distance"].is_null()) {
        sys.objectDistance = j["object_distance"].get<double>();
    }
    if (j.contains("field_angle_deg") && !j["field_angle_deg"].is_null()) {
        sys.fieldAngleDeg = j["field_angle_deg"].get<double>();
    }
    if (j.contains("object_height") && !j["object_height"].is_null()) {
        sys.objectHeight = j["object_height"].get<double>();
    }

    if (!j.contains("entrance_pupil_position")) {
        throw std::runtime_error("Missing required field: entrance_pupil_position");
    }
    sys.entrancePupilPosition = j["entrance_pupil_position"].get<double>();

    if (!j.contains("entrance_pupil_diameter")) {
        throw std::runtime_error("Missing required field: entrance_pupil_diameter");
    }
    sys.entrancePupilDiameter = j["entrance_pupil_diameter"].get<double>();
    if (sys.entrancePupilDiameter <= 0) {
        throw std::runtime_error("entrance_pupil_diameter must be positive");
    }

    sys.stopSurface = j.value("stop_surface", 1);

    if (!j.contains("field_ratios") || !j["field_ratios"].is_array()) {
        throw std::runtime_error("Missing required field: field_ratios");
    }
    for (const auto& fr : j["field_ratios"]) {
        sys.fieldRatios.push_back(fr.get<double>());
    }

    if (!j.contains("aperture_ratios") || !j["aperture_ratios"].is_array()) {
        throw std::runtime_error("Missing required field: aperture_ratios");
    }
    for (const auto& ar : j["aperture_ratios"]) {
        sys.apertureRatios.push_back(ar.get<double>());
    }

    if (!j.contains("wavelength_types") || !j["wavelength_types"].is_array()) {
        throw std::runtime_error("Missing required field: wavelength_types");
    }
    for (const auto& wt : j["wavelength_types"]) {
        sys.wavelengthTypes.push_back(wavelengthFromString(wt.get<std::string>()));
    }

    if (!j.contains("materials") || !j["materials"].is_object()) {
        throw std::runtime_error("Missing required field: materials");
    }
    for (auto& [name, matJson] : j["materials"].items()) {
        Material m;
        m.name = name;
        m.nd = parseNumericValue(matJson["nd"], "materials." + name + ".nd");
        m.nF = parseNumericValue(matJson["nF"], "materials." + name + ".nF");
        m.nC = parseNumericValue(matJson["nC"], "materials." + name + ".nC");
        sys.materials[name] = m;
    }

    if (!j.contains("surfaces") || !j["surfaces"].is_array()) {
        throw std::runtime_error("Missing required field: surfaces");
    }
    for (const auto& s : j["surfaces"]) {
        Surface surf;
        surf.radius = parseRadiusValue(s["radius"]);
        surf.thickness = s.value("thickness", 0.0);
        surf.materialAfter = s.value("material_after", "AIR");
        sys.surfaces.push_back(surf);
    }

    if (sys.surfaces.empty()) {
        throw std::runtime_error("No surfaces defined");
    }

    for (const auto& s : sys.surfaces) {
        if (sys.materials.find(s.materialAfter) == sys.materials.end()) {
            throw std::runtime_error("Material '" + s.materialAfter + "' referenced in surfaces but not defined in materials");
        }
    }

    return sys;
}

static std::string doubleToStr(double v) {
    if (std::isnan(v)) return "NaN";
    if (std::isinf(v)) {
        return v > 0 ? " Inf" : "-Inf";
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(12) << v;
    std::string s = oss.str();
    return s;
}

void saveResultsToCsv(const std::vector<ResultItem>& results, const std::string& path) {
    std::ofstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Cannot write to file: " + path);
    }

    f << "# optical_raytrace output\n";
    f << "case      order  name_zh                              value           unit\n";
    f << "--------  -----  ----------------------------------  ---------------  ----\n";

    for (const auto& r : results) {
        std::ostringstream line;
        line << std::left  << std::setw(9)  << r.caseName
             << " " << std::right << std::setw(4) << r.orderIndex
             << " " << std::left  << std::setw(34) << r.nameZh
             << " " << std::right << std::setw(15) << doubleToStr(r.value)
             << " " << std::left  << std::setw(4)  << r.unit
             << "\n";
        f << line.str();
    }
}

void saveResultsToJson(const std::vector<ResultItem>& results, const std::string& path) {
    json j = json::array();
    for (const auto& r : results) {
        json item;
        item["case"] = r.caseName;
        item["order_index"] = r.orderIndex;
        item["category"] = r.category;
        item["name_zh"] = r.nameZh;
        item["name"] = r.name;
        if (std::isnan(r.value)) {
            item["value"] = "NaN";
        } else if (std::isinf(r.value)) {
            item["value"] = (r.value > 0) ? "Inf" : "-Inf";
        } else {
            item["value"] = r.value;
        }
        item["unit"] = r.unit;
        item["zemax_reference"] = r.zemaxReference;
        item["wavelength"] = r.wavelength;
        item["field_ratio"] = r.fieldRatio;
        item["aperture_ratio"] = r.apertureRatio;
        item["description"] = r.description;
        j.push_back(item);
    }

    std::ofstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Cannot write to file: " + path);
    }
    f << j.dump(2) << "\n";
}

} // namespace optical
