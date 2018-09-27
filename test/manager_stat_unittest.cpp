#include "blob_mock.hpp"

#include <blobs-ipmid/manager.hpp>

#include <gtest/gtest.h>

namespace blobs
{

namespace
{

BlobMock* currentHandler;

GenericBlobInterface* CreateBlobMock()
{
    return currentHandler;
}

} // namespace

using ::testing::Return;

TEST(ManagerStatTest, StatNoHandler)
{
    // There is no handler for this path.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    currentHandler = m1ptr;
    EXPECT_TRUE(mgr.registerHandler(CreateBlobMock));

    struct BlobMeta meta;
    std::string path = "/asdf/asdf";
    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(false));

    EXPECT_FALSE(mgr.stat(path, &meta));
}

TEST(ManagerStatTest, StatHandlerFoundButFails)
{
    // There is a handler for this path but Stat fails.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    currentHandler = m1ptr;
    EXPECT_TRUE(mgr.registerHandler(CreateBlobMock));

    struct BlobMeta meta;
    std::string path = "/asdf/asdf";
    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(true));
    EXPECT_CALL(*m1ptr, stat(path, &meta)).WillOnce(Return(false));

    EXPECT_FALSE(mgr.stat(path, &meta));
}

TEST(ManagerStatTest, StatHandlerFoundAndSucceeds)
{
    // There is a handler and Stat succeeds.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    currentHandler = m1ptr;
    EXPECT_TRUE(mgr.registerHandler(CreateBlobMock));

    struct BlobMeta meta;
    std::string path = "/asdf/asdf";
    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(true));
    EXPECT_CALL(*m1ptr, stat(path, &meta)).WillOnce(Return(true));

    EXPECT_TRUE(mgr.stat(path, &meta));
}
} // namespace blobs
