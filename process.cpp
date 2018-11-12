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
#include <unordered_map>
#include <vector>

namespace blobs
{

/* Used by all commands with data. */
struct BmcRx
{
    uint8_t cmd;
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

IpmiBlobHandler validateBlobCommand(CrcInterface* crc, const uint8_t* reqBuf,
                                    uint8_t* replyCmdBuf, size_t* dataLen,
                                    ipmi_ret_t* code)
{
    size_t requestLength = (*dataLen);
    /* We know dataLen is at least 1 already */
    auto command = static_cast<BlobOEMCommands>(reqBuf[0]);

    /* Validate it's at least well-formed. */
    if (!validateRequestLength(command, requestLength))
    {
        *code = IPMI_CC_REQ_DATA_LEN_INVALID;
        return nullptr;
    }

    /* If there is a payload. */
    if (requestLength > sizeof(uint8_t))
    {
        /* Verify the request includes: command, crc16, data */
        if (requestLength < sizeof(struct BmcRx))
        {
            *code = IPMI_CC_REQ_DATA_LEN_INVALID;
            return nullptr;
        }

        /* We don't include the command byte at offset 0 as part of the crc
         * payload area or the crc bytes at the beginning.
         */
        size_t requestBodyLen = requestLength - 3;

        /* We start after the command byte. */
        std::vector<uint8_t> bytes(requestBodyLen);

        /* It likely has a well-formed payload. */
        struct BmcRx request;
        std::memcpy(&request, reqBuf, sizeof(request));
        uint16_t crcValue = request.crc;

        /* Set the in-place CRC to zero. */
        std::memcpy(bytes.data(), &reqBuf[3], requestBodyLen);

        crc->clear();
        crc->compute(bytes.data(), bytes.size());

        /* Crc expected but didn't match. */
        if (crcValue != crc->get())
        {
            *code = IPMI_CC_UNSPECIFIED_ERROR;
            return nullptr;
        }
    }

    /* Grab the corresponding handler for the command. */
    auto found = handlers.find(command);
    if (found == handlers.end())
    {
        *code = IPMI_CC_INVALID_FIELD_REQUEST;
        return nullptr;
    }

    return found->second;
}

ipmi_ret_t processBlobCommand(IpmiBlobHandler cmd, ManagerInterface* mgr,
                              CrcInterface* crc, const uint8_t* reqBuf,
                              uint8_t* replyCmdBuf, size_t* dataLen)
{
    ipmi_ret_t result = cmd(mgr, reqBuf, replyCmdBuf, dataLen);
    if (result != IPMI_CC_OK)
    {
        return result;
    }

    size_t replyLength = (*dataLen);

    /* The command, whatever it was, returned success. */
    if (replyLength == 0)
    {
        return result;
    }

    /* The response, if it has one byte, has three, to include the crc16. */
    if (replyLength < (sizeof(uint16_t) + 1))
    {
        return IPMI_CC_INVALID;
    }

    /* The command, whatever it was, replied, so let's set the CRC. */
    crc->clear();
    replyCmdBuf[0] = 0x00;
    replyCmdBuf[1] = 0x00;
    crc->compute(replyCmdBuf, replyLength);

    /* Copy the CRC into place. */
    uint16_t crcValue = crc->get();
    std::memcpy(replyCmdBuf, &crcValue, sizeof(crcValue));

    return result;
}
} // namespace blobs
