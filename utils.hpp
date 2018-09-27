#pragma once

#include <string>
#include <vector>

namespace blobs
{

/**
 * @brief Select all files ending with .so in a given direntry.
 *
 * @param[in] name - path
 * @return true if it matches criteria, false otherwise.
 */
bool isLibrary(const std::string& name);

/**
 * @brief Given a path, find libraries (*.so only) and load them.
 *
 * @param[in] paths - list of fully qualified paths to libraries to load.
 */
void loadLibraries(const std::string& path);

} // namespace blobs
