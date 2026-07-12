#ifndef ABERRATIONS_HPP
#define ABERRATIONS_HPP

#include "Models.hpp"

namespace optical {

std::vector<ResultItem> compute38Values(
    const OpticalSystem& system,
    const std::string& caseName
);

std::vector<ResultItem> compute76Values(
    const OpticalSystem& infinitySystem,
    const OpticalSystem& finiteSystem
);

const std::vector<ResultSpec>& getOutputSpecs();

} // namespace optical

#endif // ABERRATIONS_HPP
