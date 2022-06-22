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

#include "config.h"

#include "ipmi.hpp"
#include "manager.hpp"
#include "process.hpp"
#include "utils.hpp"

#include <ipmid/api.h>

#include <cstdio>
#include <ipmid/api-types.hpp>
#include <ipmid/handler.hpp>
#include <ipmid/iana.hpp>
#include <ipmid/oemopenbmc.hpp>
#include <ipmid/oemrouter.hpp>
#include <memory>
#include <phosphor-logging/log.hpp>
#include <span>
#include <user_channel/channel_layer.hpp>

namespace blobs
{

using namespace phosphor::logging;

void setupBlobGlobalHandler() __attribute__((constructor));

void setupBlobGlobalHandler()
{
    std::fprintf(stderr,
                 "Registering OEM:[%#08X], Cmd:[%#04X] for Blob Commands\n",
                 oem::obmcOemNumber, oem::Cmd::blobTransferCmd);

    ipmi::registerOemHandler(
        ipmi::prioOemBase, oem::obmcOemNumber, oem::Cmd::blobTransferCmd,
        ::ipmi::Privilege::User,
        [](ipmi::Context::ptr ctx, uint8_t cmd,
           const std::vector<uint8_t>& data) {
            // Get current IPMI channel and get the max transfer size
            // (assuming that it does not change).
            return handleBlobCommand(
                cmd, data, ipmi::getChannelMaxTransferSize(ctx->channel));
        });

    /* Install handlers. */
    try
    {
        loadLibraries(getBlobManager(), BLOB_LIB_PATH);
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("ERROR loading blob handlers",
                        entry("ERROR=%s", e.what()));
    }
}
} // namespace blobs
