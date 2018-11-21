/*
 * Copyright 2018 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

using HandlerFactory = std::unique_ptr<GenericBlobInterface> (*)();

void loadLibraries(ManagerInterface* manager, const std::string& path,
                   const internal::DlSysInterface* sys)
{
    void* libHandle = NULL;
    HandlerFactory factory;

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

        libHandle = sys->dlopen(ps.c_str(), RTLD_NOW | RTLD_GLOBAL);
        if (!libHandle)
        {
            log<level::ERR>("ERROR opening", entry("HANDLER=%s", ps.c_str()),
                            entry("ERROR=%s", sys->dlerror()));
            continue;
        }

        sys->dlerror(); /* Clear any previous error. */

        factory = reinterpret_cast<HandlerFactory>(
            sys->dlsym(libHandle, "createHandler"));

        const char* error = sys->dlerror();
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
