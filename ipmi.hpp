#pragma once

#include "manager.hpp"

#include <ipmid/api.h>

#include <blobs-ipmid/blobs.hpp>
#include <ipmid/api-types.hpp>
#include <span>
#include <string>
#include <vector>

namespace blobs
{

using Resp = ipmi::RspType<std::vector<uint8_t>>;

/* Used by bmcBlobGetCount */
struct BmcBlobCountTx
{
} __attribute__((packed));

struct BmcBlobCountRx
{
    uint16_t crc;
    uint32_t blobCount;
} __attribute__((packed));

/* Used by bmcBlobEnumerate */
struct BmcBlobEnumerateTx
{
    uint16_t crc;
    uint32_t blobIdx;
} __attribute__((packed));

struct BmcBlobEnumerateRx
{
    uint16_t crc;
} __attribute__((packed));

/* Used by bmcBlobOpen */
struct BmcBlobOpenTx
{
    uint16_t crc;
    uint16_t flags;
} __attribute__((packed));

struct BmcBlobOpenRx
{
    uint16_t crc;
    uint16_t sessionId;
} __attribute__((packed));

/* Used by bmcBlobClose */
struct BmcBlobCloseTx
{
    uint16_t crc;
    uint16_t sessionId; /* Returned from BmcBlobOpen. */
} __attribute__((packed));

/* Used by bmcBlobDelete */
struct BmcBlobDeleteTx
{
    uint16_t crc;
} __attribute__((packed));

/* Used by bmcBlobStat */
struct BmcBlobStatTx
{
    uint16_t crc;
} __attribute__((packed));

struct BmcBlobStatRx
{
    uint16_t crc;
    uint16_t blobState;
    uint32_t size; /* Size in bytes of the blob. */
    uint8_t metadataLen;
} __attribute__((packed));

/* Used by bmcBlobSessionStat */
struct BmcBlobSessionStatTx
{
    uint16_t crc;
    uint16_t sessionId;
} __attribute__((packed));

/* Used by bmcBlobCommit */
struct BmcBlobCommitTx
{
    uint16_t crc;
    uint16_t sessionId;
    uint8_t commitDataLen;
} __attribute__((packed));

/* Used by bmcBlobRead */
struct BmcBlobReadTx
{
    uint16_t crc;
    uint16_t sessionId;
    uint32_t offset;        /* The byte sequence start, 0-based. */
    uint32_t requestedSize; /* The number of bytes requested for reading. */
} __attribute__((packed));

struct BmcBlobReadRx
{
    uint16_t crc;
} __attribute__((packed));

/* Used by bmcBlobWrite */
struct BmcBlobWriteTx
{
    uint16_t crc;
    uint16_t sessionId;
    uint32_t offset; /* The byte sequence start, 0-based. */
} __attribute__((packed));

/* Used by bmcBlobWriteMeta */
struct BmcBlobWriteMetaTx
{
    uint16_t crc;
    uint16_t sessionId; /* Returned from BmcBlobOpen. */
    uint32_t offset;    /* The byte sequence start, 0-based. */
} __attribute__((packed));

/**
 * Validate the minimum request length if there is one.
 *
 * @param[in] subcommand - the command
 * @param[in] requestLength - the length of the request
 * @return bool - true if valid.
 */
bool validateRequestLength(BlobOEMCommands command, size_t requestLen);

/**
 * Given a pointer into an IPMI request buffer and the length of the remaining
 * buffer, builds a string.  This does no string validation w.r.t content.
 *
 * @param[in] data - Buffer containing the string.
 * @return the string if valid otherwise an empty string.
 */
std::string stringFromBuffer(std::span<const uint8_t> data);

/**
 * Writes out a BmcBlobCountRx structure and returns IPMI_OK.
 */
Resp getBlobCount(ManagerInterface* mgr, std::span<const uint8_t> data);

/**
 * Writes out a BmcBlobEnumerateRx in response to a BmcBlobEnumerateTx
 * request.  If the index does not correspond to a blob, then this will
 * return failure.
 *
 * It will also return failure if the response buffer is of an invalid
 * length.
 */
Resp enumerateBlob(ManagerInterface* mgr, std::span<const uint8_t> data);

/**
 * Attempts to open the blobId specified and associate with a session id.
 */
Resp openBlob(ManagerInterface* mgr, std::span<const uint8_t> data);

/**
 * Attempts to close the session specified.
 */
Resp closeBlob(ManagerInterface* mgr, std::span<const uint8_t> data);

/**
 * Attempts to delete the blobId specified.
 */
Resp deleteBlob(ManagerInterface* mgr, std::span<const uint8_t> data);

/**
 * Attempts to retrieve the Stat for the blobId specified.
 */
Resp statBlob(ManagerInterface* mgr, std::span<const uint8_t> data);

/**
 * Attempts to retrieve the Stat for the session specified.
 */
Resp sessionStatBlob(ManagerInterface* mgr, std::span<const uint8_t> data);

/**
 * Attempts to commit the data in the blob.
 */
Resp commitBlob(ManagerInterface* mgr, std::span<const uint8_t> data);

/**
 * Attempt to read data from the blob.
 */
Resp readBlob(ManagerInterface* mgr, std::span<const uint8_t> data);

/**
 * Attempt to write data to the blob.
 */
Resp writeBlob(ManagerInterface* mgr, std::span<const uint8_t> data);

/**
 * Attempt to write metadata to the blob.
 */
Resp writeMeta(ManagerInterface* mgr, std::span<const uint8_t> data);

} // namespace blobs
