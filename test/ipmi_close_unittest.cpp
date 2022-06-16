#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <cstring>
#include <string>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::Return;

TEST(BlobCloseTest, ManagerRejectsCloseReturnsFailure)
{
    // The session manager returned failure to close, which we need to pass on.

    ManagerMock mgr;
    uint16_t sessionId = 0x54;
    size_t dataLen;
    std::vector<uint8_t> request;
    struct BmcBlobCloseTx req;

    req.crc = 0;
    req.sessionId = sessionId;

    dataLen = sizeof(req);
    request.resize(dataLen);
    std::memcpy(request.data(), &req, dataLen);

    EXPECT_CALL(mgr, close(sessionId)).WillOnce(Return(false));
    EXPECT_EQ(ipmi::responseUnspecifiedError(), closeBlob(&mgr, request));
}

TEST(BlobCloseTest, BlobClosedReturnsSuccess)
{
    // Verify that if all goes right, success is returned.

    ManagerMock mgr;
    uint16_t sessionId = 0x54;
    size_t dataLen;
    std::vector<uint8_t> request;
    struct BmcBlobCloseTx req;

    req.crc = 0;
    req.sessionId = sessionId;

    dataLen = sizeof(req);
    request.resize(dataLen);
    std::memcpy(request.data(), &req, dataLen);

    EXPECT_CALL(mgr, close(sessionId)).WillOnce(Return(true));
    EXPECT_EQ(ipmi::responseSuccess(std::vector<uint8_t>{}),
              closeBlob(&mgr, request));
}
} // namespace blobs
