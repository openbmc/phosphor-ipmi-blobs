#include "internal.hpp"

#include "utils.hpp"

#include <string>

namespace blobs
{

void installHandlers(const std::string& path)
{
    /* May seem silly but lets us do some testing. */
    return loadLibraries(getLibraries(path));
}

} // namespace blobs
