#ifndef IO_UTILS_HPP
#define IO_UTILS_HPP

#include "Models.hpp"

#include <string>
#include <vector>

namespace optical {

OpticalSystem loadSystemFromJson(const std::string& path);
void saveResultsToCsv(const std::vector<ResultItem>& results, const std::string& path);
void saveResultsToJson(const std::vector<ResultItem>& results, const std::string& path);

} // namespace optical

#endif // IO_UTILS_HPP
