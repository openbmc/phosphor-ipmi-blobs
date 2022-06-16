#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <cstring>
#include <string>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::Return;
using ::testing::StrEq;

TEST(BlobDeleteTest, InvalidRequestLengthReturnsFailure)
{
    // There is a minimum blobId length of one character, this test verifies
    // we check that.
    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobDeleteTx req;
    req.crc = 0;
    std::string blobId = "abc";

    request.resize(sizeof(struct BmcBlobDeleteTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobDeleteTx));
    request.insert(request.end(), blobId.begin(), blobId.end());

    EXPECT_EQ(ipmi::responseReqDataLenInvalid(), deleteBlob(&mgr, request));
}

TEST(BlobDeleteTest, RequestRejectedReturnsFailure)
{
    // The blobId is rejected for any reason.
    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobDeleteTx req;
    req.crc = 0;
    std::string blobId = "a";

    request.resize(sizeof(struct BmcBlobDeleteTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobDeleteTx));
    request.insert(request.end(), blobId.begin(), blobId.end());
    request.emplace_back('\0');

    EXPECT_CALL(mgr, deleteBlob(StrEq(blobId))).WillOnce(Return(false));
    EXPECT_EQ(ipmi::responseUnspecifiedError(), deleteBlob(&mgr, request));
}

TEST(BlobDeleteTest, BlobDeleteReturnsOk)
{
    // The boring case where the blobId is deleted.
    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobDeleteTx req;
    req.crc = 0;
    std::string blobId = "a";

    request.resize(sizeof(struct BmcBlobDeleteTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobDeleteTx));
    request.insert(request.end(), blobId.begin(), blobId.end());
    request.emplace_back('\0');

    EXPECT_CALL(mgr, deleteBlob(StrEq(blobId))).WillOnce(Return(true));

    EXPECT_EQ(ipmi::responseSuccess(std::vector<uint8_t>{}),
              deleteBlob(&mgr, request));
}
} // namespace blobs
