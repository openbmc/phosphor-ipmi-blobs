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

#include <host-ipmid/ipmid-api.h>

#include <cstdio>
#include <host-ipmid/iana.hpp>
#include <host-ipmid/oemrouter.hpp>
#include <memory>

#if ENABLE_EXAMPLE
#include "example/example.hpp"
#endif

/* TODO: Swap out once https://gerrit.openbmc-project.xyz/12743 is merged */
namespace oem
{
constexpr auto blobTransferCmd = 128;
} // namespace oem

namespace blobs
{

static ipmi_ret_t handleBlobCommand(ipmi_cmd_t cmd, const uint8_t* reqBuf,
                                    uint8_t* replyCmdBuf, size_t* dataLen)
{
    /* It's holding at least a sub-command.  The OEN is trimmed from the bytes
     * before this is called.
     */
    if ((*dataLen) < 1)
    {
        return IPMI_CC_INVALID;
    }

    Crc16 crc;
    IpmiBlobHandler command =
        validateBlobCommand(&crc, reqBuf, replyCmdBuf, dataLen);
    if (command == nullptr)
    {
        return IPMI_CC_INVALID;
    }

    BlobManager* manager = getBlobManager();
    return processBlobCommand(command, manager, &crc, reqBuf, replyCmdBuf,
                              dataLen);
}

void setupBlobGlobalHandler() __attribute__((constructor));

void setupBlobGlobalHandler()
{
    oem::Router* oemRouter = oem::mutableRouter();
    std::fprintf(stderr,
                 "Registering OEM:[%#08X], Cmd:[%#04X] for Blob Commands\n",
                 oem::obmcOemNumber, oem::blobTransferCmd);

    oemRouter->registerHandler(oem::obmcOemNumber, oem::blobTransferCmd,
                               handleBlobCommand);

#if ENABLE_EXAMPLE
    BlobManager* manager = getBlobManager();
    manager->registerHandler(BuildExampleHandler);
#endif
}
} // namespace blobs
