#include "helper.hpp"
#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <cstring>
#include <string>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::_;
using ::testing::Invoke;
using ::testing::Matcher;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::StrEq;

// ipmid.hpp isn't installed where we can grab it and this value is per BMC
// SoC.
#define MAX_IPMI_BUFFER 64

TEST(BlobStatTest, InvalidRequestLengthReturnsFailure)
{
    // There is a minimum blobId length of one character, this test verifies
    // we check that.
    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobStatTx req;
    std::string blobId = "abc";

    req.crc = 0;
    request.resize(sizeof(struct BmcBlobStatTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobStatTx));
    // Do not include the nul-terminator
    request.insert(request.end(), blobId.begin(), blobId.end());

    EXPECT_EQ(ipmi::responseReqDataLenInvalid(), statBlob(&mgr, request));
}

TEST(BlobStatTest, RequestRejectedReturnsFailure)
{
    // The blobId is rejected for any reason.

    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobStatTx req;
    std::string blobId = "a";

    req.crc = 0;
    request.resize(sizeof(struct BmcBlobStatTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobStatTx));
    request.insert(request.end(), blobId.begin(), blobId.end());
    request.emplace_back('\0');

    EXPECT_CALL(mgr, stat(Matcher<const std::string&>(StrEq(blobId)),
                          Matcher<BlobMeta*>(_)))
        .WillOnce(Return(false));

    EXPECT_EQ(ipmi::responseUnspecifiedError(), statBlob(&mgr, request));
}

TEST(BlobStatTest, RequestSucceedsNoMetadata)
{
    // Stat request succeeeds but there were no metadata bytes.

    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobStatTx req;
    std::string blobId = "a";

    req.crc = 0;
    request.resize(sizeof(struct BmcBlobStatTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobStatTx));
    request.insert(request.end(), blobId.begin(), blobId.end());
    request.emplace_back('\0');

    struct BmcBlobStatRx rep;
    rep.crc = 0x00;
    rep.blobState = 0x01;
    rep.size = 0x100;
    rep.metadataLen = 0x00;

    uint16_t blobState = rep.blobState;
    uint32_t size = rep.size;

    EXPECT_CALL(mgr, stat(Matcher<const std::string&>(StrEq(blobId)),
                          Matcher<BlobMeta*>(NotNull())))
        .WillOnce(Invoke([&](const std::string&, BlobMeta* meta) {
        meta->blobState = blobState;
        meta->size = size;
        return true;
    }));

    auto result = validateReply(statBlob(&mgr, request));

    EXPECT_EQ(sizeof(rep), result.size());
    EXPECT_EQ(0, std::memcmp(result.data(), &rep, sizeof(rep)));
}

TEST(BlobStatTest, RequestSucceedsWithMetadata)
{
    // Stat request succeeds and there were metadata bytes.

    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobStatTx req;
    std::string blobId = "a";

    req.crc = 0;
    request.resize(sizeof(struct BmcBlobStatTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobStatTx));
    request.insert(request.end(), blobId.begin(), blobId.end());
    request.emplace_back('\0');

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

    EXPECT_CALL(mgr, stat(Matcher<const std::string&>(StrEq(blobId)),
                          Matcher<BlobMeta*>(NotNull())))
        .WillOnce(Invoke([&](const std::string&, BlobMeta* meta) {
        (*meta) = lmeta;
        return true;
    }));

    auto result = validateReply(statBlob(&mgr, request));

    EXPECT_EQ(sizeof(rep) + lmeta.metadata.size(), result.size());
    EXPECT_EQ(0, std::memcmp(result.data(), &rep, sizeof(rep)));
    EXPECT_EQ(0, std::memcmp(result.data() + sizeof(rep), lmeta.metadata.data(),
                             lmeta.metadata.size()));
}
} // namespace blobs
