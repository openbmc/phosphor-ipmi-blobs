#include "helper.hpp"
#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <cstring>
#include <vector>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::Return;

TEST(BlobReadTest, ManagerReturnsNoData)
{
    // Verify that if no data is returned the IPMI command reply has no
    // payload.  The manager, in all failures, will just return 0 bytes.
    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobReadTx req;

    req.crc = 0;
    req.sessionId = 0x54;
    req.offset = 0x100;
    req.requestedSize = 0x10;
    request.resize(sizeof(struct BmcBlobReadTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobReadTx));
    std::vector<uint8_t> data;

    EXPECT_CALL(mgr, read(req.sessionId, req.offset, req.requestedSize))
        .WillOnce(Return(data));

    auto result = validateReply(readBlob(&mgr, request));
    EXPECT_EQ(sizeof(struct BmcBlobReadRx), result.size());
}

TEST(BlobReadTest, ManagerReturnsData)
{
    // Verify that if data is returned, it's placed in the expected location.
    ManagerMock mgr;
    std::vector<uint8_t> request;
    struct BmcBlobReadTx req;

    req.crc = 0;
    req.sessionId = 0x54;
    req.offset = 0x100;
    req.requestedSize = 0x10;
    request.resize(sizeof(struct BmcBlobReadTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobReadTx));
    std::vector<uint8_t> data = {0x02, 0x03, 0x05, 0x06};

    EXPECT_CALL(mgr, read(req.sessionId, req.offset, req.requestedSize))
        .WillOnce(Return(data));

    auto result = validateReply(readBlob(&mgr, request));
    EXPECT_EQ(sizeof(struct BmcBlobReadRx) + data.size(), result.size());
    EXPECT_EQ(0, std::memcmp(&result[sizeof(struct BmcBlobReadRx)], data.data(),
                             data.size()));
}

/* TODO(venture): We need a test that handles other checks such as if the size
 * requested won't fit into a packet response.
 */
} // namespace blobs
