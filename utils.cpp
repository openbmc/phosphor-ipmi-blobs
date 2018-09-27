#include "utils.hpp"

#include <dlfcn.h>

#include <experimental/filesystem>
#include <phosphor-logging/log.hpp>
#include <regex>

namespace blobs
{

namespace fs = std::experimental::filesystem;
using namespace phosphor::logging;

bool isLibrary(const std::string& name)
{
    return std::regex_match(name, std::regex(".+\\.so$"));
}

void loadLibraries(const std::string& path)
{
    void* libHandler = NULL;

    for (const auto& p : fs::recursive_directory_iterator(path))
    {
        if (!isLibrary(p.path()))
        {
            continue;
        }

        libHandler = dlopen(p.path().c_str(), RTLD_NOW);
        if (!libHandler)
        {
            log<level::ERR>("ERROR opening",
                            entry("HANDLER=%s", p.path().c_str()),
                            entry("ERROR=%s", dlerror()));
        }
    }
}

} // namespace blobs
