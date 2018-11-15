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
#include "process.hpp"
#include "utils.hpp"

#include <host-ipmid/ipmid-api.h>

#include <cstdio>
#include <host-ipmid/iana.hpp>
#include <host-ipmid/oemopenbmc.hpp>
#include <host-ipmid/oemrouter.hpp>
#include <memory>
#include <phosphor-logging/log.hpp>

namespace blobs
{

using namespace phosphor::logging;

static ipmi_ret_t handleBlobCommand(ipmi_cmd_t cmd, const uint8_t* reqBuf,
                                    uint8_t* replyCmdBuf, size_t* dataLen)
{
    /* It's holding at least a sub-command.  The OEN is trimmed from the bytes
     * before this is called.
     */
    if ((*dataLen) < 1)
    {
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    /* on failure rc is set to the corresponding IPMI error. */
    ipmi_ret_t rc = IPMI_CC_OK;
    Crc16 crc;
    IpmiBlobHandler command =
        validateBlobCommand(&crc, reqBuf, replyCmdBuf, dataLen, &rc);
    if (command == nullptr)
    {
        return rc;
    }

    return processBlobCommand(command, getBlobManager(), &crc, reqBuf,
                              replyCmdBuf, dataLen);
}

/* TODO: this should come from the makefile or recipe... */
constexpr auto expectedHandlerPath = "/usr/lib/blob-ipmid";

void setupBlobGlobalHandler() __attribute__((constructor));

void setupBlobGlobalHandler()
{
    oem::Router* oemRouter = oem::mutableRouter();
    std::fprintf(stderr,
                 "Registering OEM:[%#08X], Cmd:[%#04X] for Blob Commands\n",
                 oem::obmcOemNumber, oem::Cmd::blobTransferCmd);

    oemRouter->registerHandler(oem::obmcOemNumber, oem::Cmd::blobTransferCmd,
                               handleBlobCommand);

    /* Install handlers. */
    try
    {
        loadLibraries(expectedHandlerPath);
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("ERROR loading blob handlers",
                        entry("ERROR=%s", e.what()));
    }
}
} // namespace blobs
