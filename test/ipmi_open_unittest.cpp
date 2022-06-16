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
using ::testing::NotNull;
using ::testing::Return;
using ::testing::StrEq;

TEST(BlobOpenTest, InvalidRequestLengthReturnsFailure)
{
    // There is a minimum blobId length of one character, this test verifies
    // we check that.
    ManagerMock mgr;
    std::vector<uint8_t> request;
    BmcBlobOpenTx req;
    std::string blobId = "abc";

    req.crc = 0;
    req.flags = 0;

    // Missintg the nul-terminator.
    request.resize(sizeof(struct BmcBlobOpenTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobOpenTx));
    request.insert(request.end(), blobId.begin(), blobId.end());

    EXPECT_EQ(ipmi::responseReqDataLenInvalid(), openBlob(&mgr, request));
}

TEST(BlobOpenTest, RequestRejectedReturnsFailure)
{
    // The blobId is rejected for any reason.
    ManagerMock mgr;
    std::vector<uint8_t> request;
    BmcBlobOpenTx req;
    std::string blobId = "a";

    req.crc = 0;
    req.flags = 0;
    request.resize(sizeof(struct BmcBlobOpenTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobOpenTx));
    request.insert(request.end(), blobId.begin(), blobId.end());
    request.emplace_back('\0');

    EXPECT_CALL(mgr, open(req.flags, StrEq(blobId), _)).WillOnce(Return(false));

    EXPECT_EQ(ipmi::responseUnspecifiedError(), openBlob(&mgr, request));
}

TEST(BlobOpenTest, BlobOpenReturnsOk)
{
    // The boring case where the blobId opens.

    ManagerMock mgr;
    std::vector<uint8_t> request;
    BmcBlobOpenTx req;
    struct BmcBlobOpenRx rep;
    std::string blobId = "a";

    req.crc = 0;
    req.flags = 0;
    request.resize(sizeof(struct BmcBlobOpenTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobOpenTx));
    request.insert(request.end(), blobId.begin(), blobId.end());
    request.emplace_back('\0');

    uint16_t returnedSession = 0x54;

    EXPECT_CALL(mgr, open(req.flags, StrEq(blobId), NotNull()))
        .WillOnce(Invoke([&](uint16_t, const std::string&, uint16_t* session) {
            (*session) = returnedSession;
            return true;
        }));

    auto result = validateReply(openBlob(&mgr, request));

    rep.crc = 0;
    rep.sessionId = returnedSession;

    EXPECT_EQ(sizeof(rep), result.size());
    EXPECT_EQ(0, std::memcmp(result.data(), &rep, sizeof(rep)));
}
} // namespace blobs
