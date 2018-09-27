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

/**
 * @brief Given a list of paths, try to load each library.
 *
 * @param[in] paths - list of fully qualified paths to libraries to load.
 */
void loadLibraries(const std::vector<std::string>& paths);

} // namespace blobs
