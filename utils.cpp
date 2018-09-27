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
    void* libHandler = NULL;

    for (const auto& p : fs::recursive_directory_iterator(path))
    {
        auto ps = p.path().string();

        if (!std::regex_match(ps, std::regex(".+\\.so$")))
        {
            continue;
        }

        libHandler = dlopen(ps.c_str(), RTLD_NOW);
        if (!libHandler)
        {
            log<level::ERR>("ERROR opening", entry("HANDLER=%s", ps.c_str()),
                            entry("ERROR=%s", dlerror()));
        }
    }
}

} // namespace blobs
