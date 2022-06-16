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

#include "process.hpp"

#include "ipmi.hpp"

#include <cstring>
#include <ipmiblob/crc.hpp>
#include <ipmid/api-types.hpp>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

namespace blobs
{

/* Used by all commands with data. */
struct BmcRx
{
    uint16_t crc;
    uint8_t data; /* one byte minimum of data. */
} __attribute__((packed));

static const std::unordered_map<BlobOEMCommands, IpmiBlobHandler> handlers = {
    {BlobOEMCommands::bmcBlobGetCount, getBlobCount},
    {BlobOEMCommands::bmcBlobEnumerate, enumerateBlob},
    {BlobOEMCommands::bmcBlobOpen, openBlob},
    {BlobOEMCommands::bmcBlobRead, readBlob},
    {BlobOEMCommands::bmcBlobWrite, writeBlob},
    {BlobOEMCommands::bmcBlobCommit, commitBlob},
    {BlobOEMCommands::bmcBlobClose, closeBlob},
    {BlobOEMCommands::bmcBlobDelete, deleteBlob},
    {BlobOEMCommands::bmcBlobStat, statBlob},
    {BlobOEMCommands::bmcBlobSessionStat, sessionStatBlob},
    {BlobOEMCommands::bmcBlobWriteMeta, writeMeta},
};

IpmiBlobHandler validateBlobCommand(uint8_t cmd, std::span<const uint8_t> data)
{
    size_t requestLength = data.size();
    /* We know dataLen is at least 1 already */
    auto command = static_cast<BlobOEMCommands>(cmd);

    /* Validate it's at least well-formed. */
    if (!validateRequestLength(command, requestLength))
    {
        return [](ManagerInterface*, std::span<const uint8_t>) {
            return ipmi::responseReqDataLenInvalid();
        };
    }

    /* If there is a payload. */
    if (requestLength > sizeof(cmd))
    {
        /* Verify the request includes: command, crc16, data */
        if (requestLength < sizeof(struct BmcRx))
        {
            return [](ManagerInterface*, std::span<const uint8_t>) {
                return ipmi::responseReqDataLenInvalid();
            };
        }

        /* We don't include the command byte at offset 0 as part of the crc
         * payload area or the crc bytes at the beginning.
         */
        size_t requestBodyLen = requestLength - 3;

        /* We start after the command byte. */
        std::vector<uint8_t> bytes(requestBodyLen);

        /* It likely has a well-formed payload.
         * Get the first two bytes of the request for crc.
         */
        uint16_t crc;
        std::memcpy(&crc, data.data(), sizeof(crc));

        /* Set the in-place CRC to zero.
         * Remove the first two bytes for crc and get the reset of the request.
         */
        data = data.subspan(sizeof(crc));

        /* Crc expected but didn't match. */
        if (crc != ipmiblob::generateCrc(
                       std::vector<uint8_t>(data.begin(), data.end())))
        {
            return [](ManagerInterface*, std::span<const uint8_t>) {
                return ipmi::responseUnspecifiedError();
            };
        };
    }

    /* Grab the corresponding handler for the command. */
    auto found = handlers.find(command);
    if (found == handlers.end())
    {
        return [](ManagerInterface*, std::span<const uint8_t>) {
            return ipmi::responseInvalidFieldRequest();
        };
    }

    return found->second;
}

Resp processBlobCommand(IpmiBlobHandler cmd, ManagerInterface* mgr,
                        std::span<const uint8_t> data)
{
    Resp result = cmd(mgr, data);
    if (std::get<0>(result) != ipmi::ccSuccess)
    {
        return result;
    }

    std::vector<uint8_t>& response = std::get<0>(
        // std::variant<std::vector<uint8_t>>
        *std::get<1>(result));
    size_t replyLength = response.size();

    /* The command, whatever it was, returned success. */
    if (replyLength == 0)
    {
        return result;
    }

    /* Read can return 0 bytes, and just a CRC, otherwise you need a CRC and 1
     * byte, therefore the limit is 2 bytes.
     */
    if (replyLength < (sizeof(uint16_t)))
    {
        return ipmi::responseUnspecifiedError();
    }

    /* The command, whatever it was, replied, so let's set the CRC. */
    std::span<const uint8_t> responseView = response;
    responseView = responseView.subspan(sizeof(uint16_t));
    std::vector<std::uint8_t> crcBuffer(responseView.begin(),
                                        responseView.end());
    /* Copy the CRC into place. */
    uint16_t crcValue = ipmiblob::generateCrc(crcBuffer);
    std::memcpy(response.data(), &crcValue, sizeof(crcValue));

    return result;
}

Resp handleBlobCommand(uint8_t cmd, std::vector<uint8_t> data)
{
    /* on failure rc is set to the corresponding IPMI error. */
    return processBlobCommand(validateBlobCommand(cmd, data), getBlobManager(),
                              data);
}

} // namespace blobs
