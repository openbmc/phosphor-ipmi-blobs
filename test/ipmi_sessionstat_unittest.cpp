#include "helper.hpp"
#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <cstring>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::_;
using ::testing::Invoke;
using ::testing::Matcher;
using ::testing::NotNull;
using ::testing::Return;

// ipmid.hpp isn't installed where we can grab it and this value is per BMC
// SoC.
#define MAX_IPMI_BUFFER 64

TEST(BlobSessionStatTest, RequestRejectedByManagerReturnsFailure)
{
    // If the session ID is invalid, the request must fail.

    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobSessionStatTx req;
    req.crc = 0;
    req.sessionId = 0x54;

    request.resize(sizeof(struct BmcBlobSessionStatTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobSessionStatTx));

    EXPECT_CALL(mgr,
                stat(Matcher<uint16_t>(req.sessionId), Matcher<BlobMeta*>(_)))
        .WillOnce(Return(false));

    EXPECT_EQ(ipmi::responseUnspecifiedError(), sessionStatBlob(&mgr, request));
}

TEST(BlobSessionStatTest, RequestSucceedsNoMetadata)
{
    // Stat request succeeeds but there were no metadata bytes.

    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobSessionStatTx req;
    req.crc = 0;
    req.sessionId = 0x54;

    request.resize(sizeof(struct BmcBlobSessionStatTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobSessionStatTx));

    struct BmcBlobStatRx rep;
    rep.crc = 0x00;
    rep.blobState = 0x01;
    rep.size = 0x100;
    rep.metadataLen = 0x00;

    uint16_t blobState = rep.blobState;
    uint32_t size = rep.size;

    EXPECT_CALL(mgr, stat(Matcher<uint16_t>(req.sessionId),
                          Matcher<BlobMeta*>(NotNull())))
        .WillOnce(Invoke([&](uint16_t, BlobMeta* meta) {
            meta->blobState = blobState;
            meta->size = size;
            return true;
        }));

    auto result = validateReply(sessionStatBlob(&mgr, request));

    EXPECT_EQ(sizeof(rep), result.size());
    EXPECT_EQ(0, std::memcmp(result.data(), &rep, sizeof(rep)));
}

TEST(BlobSessionStatTest, RequestSucceedsWithMetadata)
{
    // Stat request succeeds and there were metadata bytes.

    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobSessionStatTx req;
    req.crc = 0;
    req.sessionId = 0x54;

    request.resize(sizeof(struct BmcBlobSessionStatTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobSessionStatTx));

    BlobMeta lmeta;
    lmeta.blobState = 0x01;
    lmeta.size = 0x100;
    lmeta.metadata.push_back(0x01);
    lmeta.metadata.push_back(0x02);
    lmeta.metadata.push_back(0x03);
    lmeta.metadata.push_back(0x04);

    struct BmcBlobStatRx rep;
    rep.crc = 0x00;
    rep.blobState = lmeta.blobState;
    rep.size = lmeta.size;
    rep.metadataLen = lmeta.metadata.size();

    EXPECT_CALL(mgr, stat(Matcher<uint16_t>(req.sessionId),
                          Matcher<BlobMeta*>(NotNull())))
        .WillOnce(Invoke([&](uint16_t, BlobMeta* meta) {
            (*meta) = lmeta;
            return true;
        }));

    auto result = validateReply(sessionStatBlob(&mgr, request));

    EXPECT_EQ(sizeof(rep) + lmeta.metadata.size(), result.size());
    EXPECT_EQ(0, std::memcmp(result.data(), &rep, sizeof(rep)));
    EXPECT_EQ(0, std::memcmp(result.data() + sizeof(rep), lmeta.metadata.data(),
                             lmeta.metadata.size()));
}
} // namespace blobs
