#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <cstring>

#include <gtest/gtest.h>

namespace blobs
{
using ::testing::ElementsAreArray;
using ::testing::Return;

// ipmid.hpp isn't installed where we can grab it and this value is per BMC
// SoC.
#define MAX_IPMI_BUFFER 64

TEST(BlobWriteMetaTest, ManagerReturnsFailureReturnsFailure)
{
    // This verifies a failure from the manager is passed back.
    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobWriteMetaTx req;

    req.crc = 0;
    req.sessionId = 0x54;
    req.offset = 0x100;

    request.resize(sizeof(struct BmcBlobWriteMetaTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobWriteMetaTx));

    std::array<uint8_t, 2> expectedBytes = {0x66, 0x67};
    request.insert(request.end(), expectedBytes.begin(), expectedBytes.end());

    EXPECT_CALL(mgr, writeMeta(req.sessionId, req.offset,
                               ElementsAreArray(expectedBytes)))
        .WillOnce(Return(false));

    EXPECT_EQ(ipmi::responseUnspecifiedError(), writeMeta(&mgr, request));
}

TEST(BlobWriteMetaTest, ManagerReturnsTrueWriteSucceeds)
{
    // The case where everything works.
    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobWriteMetaTx req;

    req.crc = 0;
    req.sessionId = 0x54;
    req.offset = 0x100;

    request.resize(sizeof(struct BmcBlobWriteMetaTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobWriteMetaTx));

    std::array<uint8_t, 2> expectedBytes = {0x66, 0x67};
    request.insert(request.end(), expectedBytes.begin(), expectedBytes.end());

    EXPECT_CALL(mgr, writeMeta(req.sessionId, req.offset,
                               ElementsAreArray(expectedBytes)))
        .WillOnce(Return(true));

    EXPECT_EQ(ipmi::responseSuccess(std::vector<uint8_t>{}),
              writeMeta(&mgr, request));
}
} // namespace blobs
