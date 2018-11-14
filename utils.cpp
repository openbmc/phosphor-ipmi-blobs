#include "utils.hpp"

#include <dlfcn.h>

#include <blobs-ipmid/manager.hpp>
#include <experimental/filesystem>
#include <memory>
#include <phosphor-logging/log.hpp>
#include <regex>

namespace blobs
{

namespace fs = std::experimental::filesystem;
using namespace phosphor::logging;

void loadLibraries(const std::string& path)
{
    void* libHandle = NULL;
    auto* manager = getBlobManager();

    for (const auto& p : fs::recursive_directory_iterator(path))
    {
        auto ps = p.path().string();

        /* The bitbake recipe symlinks the library lib*.so.? into the folder
         * only, and not the other names, .so, .so.?.?, .so.?.?.?
         *
         * Therefore only care if it's lib*.so.?
         */
        if (!std::regex_match(ps, std::regex(".+\\.so\\.\\d+$")))
        {
            continue;
        }

        libHandle = dlopen(ps.c_str(), RTLD_NOW | RTLD_GLOBAL);
        if (!libHandle)
        {
            log<level::ERR>("ERROR opening", entry("HANDLER=%s", ps.c_str()),
                            entry("ERROR=%s", dlerror()));
            continue;
        }

        dlerror(); /* Clear any previous error. */

        std::unique_ptr<GenericBlobInterface> (*factory)(void);
        factory = (std::unique_ptr<GenericBlobInterface>(*)(void))dlsym(
            libHandle, "createHandler");

        const char* error = dlerror();
        if (error)
        {
            log<level::ERR>("ERROR loading symbol",
                            entry("HANDLER=%s", ps.c_str()),
                            entry("ERROR=%s", error));
            continue;
        }

        std::unique_ptr<GenericBlobInterface> result = factory();
        if (!result)
        {
            log<level::ERR>("Unable to create handler",
                            entry("HANDLER=%s", ps.c_str()));
            continue;
        }

        manager->registerHandler(std::move(result));
    }
}

} // namespace blobs
