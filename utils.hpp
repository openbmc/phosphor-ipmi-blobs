#pragma once

#include <string>

namespace blobs
{

/**
 * @brief Given a path, find libraries (*.so only) and load them.
 *
 * @param[in] paths - list of fully qualified paths to libraries to load.
 */
void loadLibraries(const std::string& path);

} // namespace blobs
