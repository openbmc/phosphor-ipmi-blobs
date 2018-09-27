#pragma once

#include <string>
#include <vector>

namespace blobs
{

/**
 * @brief Given a path, return a list of the libraries found.
 *
 * @param[in] path - the path to search.
 * @return list of libraries.
 */
std::vector<std::string> getLibraries(const std::string& path);

} // namespace blobs
