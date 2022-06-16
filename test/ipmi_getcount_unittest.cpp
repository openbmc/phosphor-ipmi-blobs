#include "helper.hpp"
#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <cstring>
#include <vector>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::Return;

// the request here is only the subcommand byte and therefore there's no invalid
// length check, etc to handle within the method.

TEST(BlobCountTest, ReturnsZeroBlobs)
{
    // Calling BmcBlobGetCount if there are no handlers registered should just
    // return that there are 0 blobs.

    ManagerMock mgr;
    struct BmcBlobCountRx rep;

    rep.crc = 0;
    rep.blobCount = 0;

    EXPECT_CALL(mgr, buildBlobList()).WillOnce(Return(0));

    auto result = validateReply(getBlobCount(&mgr, {}));

    EXPECT_EQ(sizeof(rep), result.size());
    EXPECT_EQ(0, std::memcmp(result.data(), &rep, sizeof(rep)));
}

TEST(BlobCountTest, ReturnsTwoBlobs)
{
    // Calling BmcBlobGetCount with one handler registered that knows of two
    // blobs will return that it found two blobs.

    ManagerMock mgr;
    struct BmcBlobCountRx rep;

    rep.crc = 0;
    rep.blobCount = 2;

    EXPECT_CALL(mgr, buildBlobList()).WillOnce(Return(2));

    auto result = validateReply(getBlobCount(&mgr, {}));

    EXPECT_EQ(sizeof(rep), result.size());
    EXPECT_EQ(0, std::memcmp(result.data(), &rep, sizeof(rep)));
}
} // namespace blobs
