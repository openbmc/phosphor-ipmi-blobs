#include "utils.hpp"

#include <dirent.h>
#include <dlfcn.h>

#include <phosphor-logging/log.hpp>

namespace blobs
{

using namespace phosphor::logging;

namespace
{

static const auto soNameExtensionVersioned = ".so.";
static const std::string soNameExtension = ".so";

constexpr auto minimumLength = 4; /* a.so */

/**
 * Select all files ending with .so in a given direntry.
 * @param[in,out] entry - dirent pointer
 * @return 1 if it matches criteria, 0 otherwise.
 */
int handlerSelect(const struct dirent* entry)
{
    /* string::ends_with() is in c++20 */
    std::string name(entry->d_name);

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
    struct dirent** handlerList;
    std::vector<std::string> output;
    int numHandlers =
        scandir(path.c_str(), &handlerList, handlerSelect, alphasort);
    if (numHandlers < 0)
    {
        return output;
    }

    while (numHandlers--)
    {
        output.push_back(path + handlerList[numHandlers]->d_name);
        free(handlerList[numHandlers]);
    }
    free(handlerList);

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
