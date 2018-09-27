#include "utils.hpp"

#include <dlfcn.h>

#include <experimental/filesystem>
#include <phosphor-logging/log.hpp>

namespace blobs
{

namespace fs = std::experimental::filesystem;
using namespace phosphor::logging;

namespace
{

/* Note: no real huge value in using regex for this. */
static const auto soNameExtensionVersioned = ".so.";
static const std::string soNameExtension = ".so";

constexpr auto minimumLength = 4; /* a.so */
/**
 * Select all files ending with .so in a given direntry.
 * @param[in] name - path
 * @return 1 if it matches criteria, 0 otherwise.
 */
int handlerSelect(const std::string& name)
{
    /* string::ends_with() is in c++20 */

    /* Check for .so.* */
    if (std::string::npos != name.find(soNameExtensionVersioned))
    {
        return 1;
    }

    /* Check for ones that end in .so */
    if (name.length() < minimumLength)
    {
        return 0;
    }

    if (name.substr(name.length() - soNameExtension.length()) ==
        soNameExtension)
    {
        return 1;
    }

    return 0;
}
} // namespace

std::vector<std::string> getLibraries(const std::string& path)
{
    std::vector<std::string> output;

    /* Output will automatically be path/file */
    for (auto& p : fs::recursive_directory_iterator(path))
    {
        if (handlerSelect(p.path()))
        {
            output.push_back(p.path());
        }
    }

    return output;
}

void loadLibraries(const std::vector<std::string>& paths)
{
    void* libHandler = NULL;
    for (const auto& p : paths)
    {
        libHandler = dlopen(p.c_str(), RTLD_NOW);
        if (!libHandler)
        {
            log<level::ERR>("ERROR opening", entry("HANDLER=%s", p.c_str()),
                            entry("ERROR=%s", dlerror()));
        }
    }
}

} // namespace blobs
