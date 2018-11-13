#include "utils.hpp"

#include <dlfcn.h>

#include <experimental/filesystem>
#include <phosphor-logging/log.hpp>
#include <regex>

namespace blobs
{

namespace fs = std::experimental::filesystem;
using namespace phosphor::logging;

void loadLibraries(const std::string& path)
{
    void* libHandle = NULL;

    for (const auto& p : fs::recursive_directory_iterator(path))
    {
        auto ps = p.path().string();

        /* The bitbake recipe symlinks the library lib*.so.? into the folder only, and not the other names, .so, .so.?.?, .so.?.?.?
         *
         * Therefore only care if it's lib*.so.?
         */
        if (!std::regex_match(ps, std::regex(".+\\.so\\.\\d+$")))
        {
            continue;
        }

        libHandle = dlopen(ps.c_str(), RTLD_NOW);
        if (!libHandle)
        {
            log<level::ERR>("ERROR opening", entry("HANDLER=%s", ps.c_str()),
                            entry("ERROR=%s", dlerror()));
        }
    }
}

} // namespace blobs
