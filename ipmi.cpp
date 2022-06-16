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

#include "ipmi.hpp"

#include <cstring>
#include <span>
#include <string>
#include <unordered_map>

namespace blobs
{

bool validateRequestLength(BlobOEMCommands command, size_t requestLen)
{
    /* The smallest string is one letter and the nul-terminator. */
    static const int kMinStrLen = 2;

    static const std::unordered_map<BlobOEMCommands, size_t> minimumLengths = {
        {BlobOEMCommands::bmcBlobEnumerate, sizeof(struct BmcBlobEnumerateTx)},
        {BlobOEMCommands::bmcBlobOpen,
         sizeof(struct BmcBlobOpenTx) + kMinStrLen},
        {BlobOEMCommands::bmcBlobClose, sizeof(struct BmcBlobCloseTx)},
        {BlobOEMCommands::bmcBlobDelete,
         sizeof(struct BmcBlobDeleteTx) + kMinStrLen},
        {BlobOEMCommands::bmcBlobStat,
         sizeof(struct BmcBlobStatTx) + kMinStrLen},
        {BlobOEMCommands::bmcBlobSessionStat,
         sizeof(struct BmcBlobSessionStatTx)},
        {BlobOEMCommands::bmcBlobCommit, sizeof(struct BmcBlobCommitTx)},
        {BlobOEMCommands::bmcBlobRead, sizeof(struct BmcBlobReadTx)},
        {BlobOEMCommands::bmcBlobWrite,
         sizeof(struct BmcBlobWriteTx) + sizeof(uint8_t)},
        {BlobOEMCommands::bmcBlobWriteMeta,
         sizeof(struct BmcBlobWriteMetaTx) + sizeof(uint8_t)},
    };

    auto results = minimumLengths.find(command);
    if (results == minimumLengths.end())
    {
        /* Valid length by default if we don't care. */
        return true;
    }

    /* If the request is shorter than the minimum, it's invalid. */
    if (requestLen < results->second)
    {
        return false;
    }

    return true;
}

std::string stringFromBuffer(std::span<const uint8_t> data)
{
    if (data.empty() || data.back() != '\0')
    {
        return std::string();
    }

    // Last index is nul-terminator.
    return std::string(data.begin(), data.end() - 1);
}

Resp getBlobCount(ManagerInterface* mgr, std::span<const uint8_t>)
{
    struct BmcBlobCountRx resp;
    resp.crc = 0;
    resp.blobCount = mgr->buildBlobList();

    /* Copy the response into the reply buffer */
    std::vector<uint8_t> output(sizeof(BmcBlobCountRx), 0);
    std::memcpy(output.data(), &resp, sizeof(resp));

    return ipmi::responseSuccess(output);
}

Resp enumerateBlob(ManagerInterface* mgr, std::span<const uint8_t> data)
{
    /* Verify datalen is >= sizeof(request) */
    struct BmcBlobEnumerateTx request;

    std::memcpy(&request, data.data(), sizeof(request));

    std::string blobId = mgr->getBlobId(request.blobIdx);
    if (blobId.empty())
    {
        return ipmi::responseInvalidFieldRequest();
    }

    std::vector<uint8_t> output(sizeof(BmcBlobEnumerateRx), 0);
    output.insert(output.end(), blobId.c_str(),
                  blobId.c_str() + blobId.length() + 1);
    return ipmi::responseSuccess(output);
}

Resp openBlob(ManagerInterface* mgr, std::span<const uint8_t> data)
{
    auto request = reinterpret_cast<const struct BmcBlobOpenTx*>(data.data());
    uint16_t session;

    std::string path = stringFromBuffer(data.subspan(sizeof(BmcBlobOpenTx)));
    if (path.empty())
    {
        return ipmi::responseReqDataLenInvalid();
    }

    /* Attempt to open. */
    if (!mgr->open(request->flags, path, &session))
    {
        return ipmi::responseUnspecifiedError();
    }

    struct BmcBlobOpenRx reply;
    reply.crc = 0;
    reply.sessionId = session;

    std::vector<uint8_t> output(sizeof(BmcBlobOpenRx), 0);
    std::memcpy(output.data(), &reply, sizeof(reply));
    return ipmi::responseSuccess(output);
}

Resp closeBlob(ManagerInterface* mgr, std::span<const uint8_t> data)
{
    struct BmcBlobCloseTx request;
    std::memcpy(&request, data.data(), sizeof(request));

    /* Attempt to close. */
    if (!mgr->close(request.sessionId))
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(std::vector<uint8_t>{});
}

Resp deleteBlob(ManagerInterface* mgr, std::span<const uint8_t> data)
{
    std::string path = stringFromBuffer(data.subspan(sizeof(BmcBlobDeleteTx)));
    if (path.empty())
    {
        return ipmi::responseReqDataLenInvalid();
    }

    /* Attempt to delete. */
    if (!mgr->deleteBlob(path))
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(std::vector<uint8_t>{});
}

static Resp returnStatBlob(BlobMeta* meta)
{
    struct BmcBlobStatRx reply;
    reply.crc = 0;
    reply.blobState = meta->blobState;
    reply.size = meta->size;
    reply.metadataLen = meta->metadata.size();

    std::vector<uint8_t> output(sizeof(BmcBlobStatRx), 0);
    std::memcpy(output.data(), &reply, sizeof(reply));

    /* If there is metadata, insert it to output. */
    if (!meta->metadata.empty())
    {
        output.insert(output.end(), meta->metadata.begin(),
                      meta->metadata.end());
    }
    return ipmi::responseSuccess(output);
}

Resp statBlob(ManagerInterface* mgr, std::span<const uint8_t> data)
{
    std::string path = stringFromBuffer(data.subspan(sizeof(BmcBlobStatTx)));
    if (path.empty())
    {
        return ipmi::responseReqDataLenInvalid();
    }

    /* Attempt to stat. */
    BlobMeta meta;
    if (!mgr->stat(path, &meta))
    {
        return ipmi::responseUnspecifiedError();
    }

    return returnStatBlob(&meta);
}

Resp sessionStatBlob(ManagerInterface* mgr, std::span<const uint8_t> data)
{
    struct BmcBlobSessionStatTx request;
    std::memcpy(&request, data.data(), sizeof(request));

    /* Attempt to stat. */
    BlobMeta meta;

    if (!mgr->stat(request.sessionId, &meta))
    {
        return ipmi::responseUnspecifiedError();
    }

    return returnStatBlob(&meta);
}

Resp commitBlob(ManagerInterface* mgr, std::span<const uint8_t> data)
{
    auto request = reinterpret_cast<const struct BmcBlobCommitTx*>(data.data());

    /* Sanity check the commitDataLen */
    if (request->commitDataLen > (data.size() - sizeof(struct BmcBlobCommitTx)))
    {
        return ipmi::responseReqDataLenInvalid();
    }

    data = data.subspan(sizeof(struct BmcBlobCommitTx), request->commitDataLen);

    if (!mgr->commit(request->sessionId,
                     std::vector<uint8_t>(data.begin(), data.end())))
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(std::vector<uint8_t>{});
}

Resp readBlob(ManagerInterface* mgr, std::span<const uint8_t> data)
{
    struct BmcBlobReadTx request;
    std::memcpy(&request, data.data(), sizeof(request));

    std::vector<uint8_t> result =
        mgr->read(request.sessionId, request.offset, request.requestedSize);

    /* If the Read fails, it returns success but with only the crc and 0 bytes
     * of data.
     * If there was data returned, copy into the reply buffer.
     */
    std::vector<uint8_t> output(sizeof(BmcBlobReadRx), 0);

    if (!result.empty())
    {
        output.insert(output.end(), result.begin(), result.end());
    }

    return ipmi::responseSuccess(output);
}

Resp writeBlob(ManagerInterface* mgr, std::span<const uint8_t> data)
{
    auto request = reinterpret_cast<const struct BmcBlobWriteTx*>(data.data());
    data = data.subspan(sizeof(struct BmcBlobWriteTx));

    /* Attempt to write the bytes. */
    if (!mgr->write(request->sessionId, request->offset,
                    std::vector<uint8_t>(data.begin(), data.end())))
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(std::vector<uint8_t>{});
}

Resp writeMeta(ManagerInterface* mgr, std::span<const uint8_t> data)
{
    struct BmcBlobWriteMetaTx request;

    /* Copy over the request. */
    std::memcpy(&request, data.data(), sizeof(request));

    /* Nothing really else to validate, we just copy those bytes. */
    data = data.subspan(sizeof(struct BmcBlobWriteMetaTx));

    /* Attempt to write the bytes. */
    if (!mgr->writeMeta(request.sessionId, request.offset,
                        std::vector<uint8_t>(data.begin(), data.end())))
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(std::vector<uint8_t>{});
}

} // namespace blobs
