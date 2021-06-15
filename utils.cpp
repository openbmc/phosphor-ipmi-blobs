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

#include "fs.hpp"
#include "manager.hpp"

#include <dlfcn.h>

#include <memory>
#include <phosphor-logging/log.hpp>
#include <regex>
#include <string>
#include <unordered_set>
#include <vector>

namespace blobs
{

using namespace phosphor::logging;

bool matchBlobHandler(const std::string& filename)
{
    return filename.find(".so") != std::string::npos;
}

void loadLibraries(ManagerInterface* manager, const std::string& path,
                   const internal::DlSysInterface* sys)
{
    std::unordered_set<HandlerFactory> seen;
    void* libHandle = NULL;
    HandlerFactory factory;

    std::vector<std::string> libs = getLibraryList(path, matchBlobHandler);

    for (const auto& p : libs)
    {
        libHandle = sys->dlopen(p.c_str(), RTLD_NOW | RTLD_GLOBAL);
        if (!libHandle)
        {
            log<level::ERR>("ERROR opening", entry("HANDLER=%s", p.c_str()),
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
                            entry("HANDLER=%s", p.c_str()),
                            entry("ERROR=%s", error));
            continue;
        }

        // We may have duplicate libraries in the blobs directory that we only
        // want to initialize once.
        if (seen.count(factory) > 0)
        {
            continue;
        }
        seen.emplace(factory);

        try
        {
            std::unique_ptr<GenericBlobInterface> result = factory();
            if (!result)
            {
                log<level::ERR>("Unable to create handler",
                                entry("HANDLER=%s", p.c_str()));
                continue;
            }

            manager->registerHandler(std::move(result));
        }
        catch (const std::exception& e)
        {
            log<level::ERR>("Received exception loading handler",
                            entry("HANDLER=%s", p.c_str()),
                            entry("EXCEPTION=%s", e.what()));
        }
    }
}

} // namespace blobs
