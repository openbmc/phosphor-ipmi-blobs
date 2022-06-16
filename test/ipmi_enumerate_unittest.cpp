#include "helper.hpp"
#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <cstring>
#include <string>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::Return;

TEST(BlobEnumerateTest, VerifyIfRequestByIdInvalidReturnsFailure)
{
    // This tests to verify that if the index is invalid, it'll return failure.

    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobEnumerateTx req;
    req.blobIdx = 0;

    request.resize(sizeof(struct BmcBlobEnumerateTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobEnumerateTx));

    EXPECT_CALL(mgr, getBlobId(req.blobIdx)).WillOnce(Return(""));
    EXPECT_EQ(ipmi::responseInvalidFieldRequest(),
              enumerateBlob(&mgr, request));
}

TEST(BlobEnumerateTest, BoringRequestByIdAndReceive)
{
    // This tests that if an index into the blob_id cache is valid, the command
    // will return the blobId.

    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobEnumerateTx req;
    req.blobIdx = 0;
    std::string blobId = "/asdf";

    request.resize(sizeof(struct BmcBlobEnumerateTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobEnumerateTx));

    EXPECT_CALL(mgr, getBlobId(req.blobIdx)).WillOnce(Return(blobId));

    auto result = validateReply(enumerateBlob(&mgr, request));

    // We're expecting this as a response.
    // blobId.length + 1 + sizeof(uint16_t);
    EXPECT_EQ(blobId.length() + 1 + sizeof(uint16_t), result.size());
    EXPECT_EQ(blobId,
              // Remove crc and nul-terminator.
              std::string(result.begin() + sizeof(uint16_t), result.end() - 1));
}
} // namespace blobs
