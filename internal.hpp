#pragma once

#include <string>

namespace blobs
{

/**
 * @brief Find and load any handlers.
 *
 * @param[in] path - the library path
 */
void installHandlers(const std::string& path);

} // namespace blobs
