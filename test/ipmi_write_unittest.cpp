#include "helper.hpp"
#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <cstring>

#include <gtest/gtest.h>
namespace blobs
{

using ::testing::ElementsAreArray;
using ::testing::Return;

TEST(BlobWriteTest, ManagerReturnsFailureReturnsFailure)
{
    // This verifies a failure from the manager is passed back.
    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobWriteTx req;

    req.crc = 0;
    req.sessionId = 0x54;
    req.offset = 0x100;

    request.resize(sizeof(struct BmcBlobWriteTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobWriteTx));

    std::array<uint8_t, 2> expectedBytes = {0x66, 0x67};
    request.insert(request.end(), expectedBytes.begin(), expectedBytes.end());

    EXPECT_CALL(
        mgr, write(req.sessionId, req.offset, ElementsAreArray(expectedBytes)))
        .WillOnce(Return(false));

    EXPECT_EQ(ipmi::responseUnspecifiedError(), writeBlob(&mgr, request));
}

TEST(BlobWriteTest, ManagerReturnsTrueWriteSucceeds)
{
    // The case where everything works.

    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobWriteTx req;

    req.crc = 0;
    req.sessionId = 0x54;
    req.offset = 0x100;

    request.resize(sizeof(struct BmcBlobWriteTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobWriteTx));

    std::array<uint8_t, 2> expectedBytes = {0x66, 0x67};
    request.insert(request.end(), expectedBytes.begin(), expectedBytes.end());

    EXPECT_CALL(
        mgr, write(req.sessionId, req.offset, ElementsAreArray(expectedBytes)))
        .WillOnce(Return(true));

    EXPECT_EQ(ipmi::responseSuccess(std::vector<uint8_t>{}),
              writeBlob(&mgr, request));
}
} // namespace blobs
