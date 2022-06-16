#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <cstring>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::_;
using ::testing::ElementsAreArray;
using ::testing::Return;

TEST(BlobCommitTest, InvalidCommitDataLengthReturnsFailure)
{
    // The commit command supports an optional commit blob.  This test verifies
    // we sanity check the length of that blob.

    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobCommitTx req;
    req.crc = 0;
    req.sessionId = 0x54;
    req.commitDataLen =
        1; // It's one byte, but that's more than the packet size.

    request.resize(sizeof(struct BmcBlobCommitTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobCommitTx));
    EXPECT_EQ(ipmi::responseReqDataLenInvalid(), commitBlob(&mgr, request));
}

TEST(BlobCommitTest, ValidCommitNoDataHandlerRejectsReturnsFailure)
{
    // The commit packet is valid and the manager's commit call returns failure.

    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobCommitTx req;
    req.crc = 0;
    req.sessionId = 0x54;
    req.commitDataLen = 0;

    request.resize(sizeof(struct BmcBlobCommitTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobCommitTx));

    EXPECT_CALL(mgr, commit(req.sessionId, _)).WillOnce(Return(false));
    EXPECT_EQ(ipmi::responseUnspecifiedError(), commitBlob(&mgr, request));
}

TEST(BlobCommitTest, ValidCommitNoDataHandlerAcceptsReturnsSuccess)
{
    // Commit called with no data and everything returns success.

    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobCommitTx req;
    req.crc = 0;
    req.sessionId = 0x54;
    req.commitDataLen = 0;

    request.resize(sizeof(struct BmcBlobCommitTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobCommitTx));
    EXPECT_CALL(mgr, commit(req.sessionId, _)).WillOnce(Return(true));

    EXPECT_EQ(ipmi::responseSuccess(), commitBlob(&mgr, request));
}

TEST(BlobCommitTest, ValidCommitWithDataHandlerAcceptsReturnsSuccess)
{
    // Commit called with extra data and everything returns success.

    ManagerMock mgr;
    std::vector<uint8_t> request;
    std::array<uint8_t, 4> expectedBlob = {0x25, 0x33, 0x45, 0x67};
    struct BmcBlobCommitTx req;
    req.crc = 0;
    req.sessionId = 0x54;
    req.commitDataLen = sizeof(expectedBlob);

    request.resize(sizeof(struct BmcBlobCommitTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobCommitTx));
    request.insert(request.end(), expectedBlob.begin(), expectedBlob.end());

    EXPECT_CALL(mgr, commit(req.sessionId, ElementsAreArray(expectedBlob)))
        .WillOnce(Return(true));

    EXPECT_EQ(ipmi::responseSuccess(), commitBlob(&mgr, request));
}
} // namespace blobs
