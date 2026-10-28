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

#include <phosphor-logging/lg2.hpp>

#include <memory>
#include <regex>
#include <string>
#include <unordered_set>
#include <vector>

PHOSPHOR_LOG2_USING;

namespace blobs
{

bool matchBlobHandler(const std::string& filename)
{
    return filename.find(".so") != std::string::npos;
}

void loadLibraries(ManagerInterface* manager, const std::string& path,
                   const internal::DlSysInterface* sys)
{
    std::unordered_set<HandlerFactory> seen;
    void* libHandle = nullptr;
    HandlerFactory factory;

    std::vector<std::string> libs = getLibraryList(path, matchBlobHandler);

    for (const auto& p : libs)
    {
        try
        {
            libHandle = sys->dlopen(p.c_str(), RTLD_NOW | RTLD_GLOBAL);
            if (!libHandle)
            {
                const char* openError = sys->dlerror();
                lg2::error("Error opening handler {HANDLER}: {ERROR}",
                           "HANDLER", p, "ERROR",
                           openError ? openError : "unknown error");
                continue;
            }

            sys->dlerror(); /* Clear any previous error. */

            factory = reinterpret_cast<HandlerFactory>(
                sys->dlsym(libHandle, "createHandler"));

            const char* error = sys->dlerror();
            if (error || !factory)
            {
                lg2::error(
                    "Error loading symbol from handler {HANDLER}: {ERROR}",
                    "HANDLER", p, "ERROR", error ? error : "null symbol");
                continue;
            }

            // We may have duplicate libraries in the blobs directory that we
            // only want to initialize once.
            if (seen.count(factory) > 0)
            {
                continue;
            }
            seen.emplace(factory);

            std::unique_ptr<GenericBlobInterface> result = factory();
            if (!result)
            {
                lg2::error("Unable to create handler {HANDLER}", "HANDLER", p);
                continue;
            }

            manager->registerHandler(std::move(result));
        }
        catch (const std::exception& e)
        {
            lg2::error(
                "Received exception loading handler {HANDLER}: {EXCEPTION}",
                "HANDLER", p, "EXCEPTION", e.what());
        }
        catch (...)
        {
            lg2::error("Received unknown exception loading handler {HANDLER}",
                       "HANDLER", p);
        }
    }
}

} // namespace blobs
