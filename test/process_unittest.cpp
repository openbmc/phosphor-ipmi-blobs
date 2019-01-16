#include "crc.hpp"
#include "crc_mock.hpp"
#include "ipmi.hpp"
#include "manager_mock.hpp"
#include "process.hpp"

#include <cstring>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;

// ipmid.hpp isn't installed where we can grab it and this value is per BMC
// SoC.
#define MAX_IPMI_BUFFER 64

namespace
{

void EqualFunctions(IpmiBlobHandler lhs, IpmiBlobHandler rhs)
{
    EXPECT_FALSE(lhs == nullptr);
    EXPECT_FALSE(rhs == nullptr);

    ipmi_ret_t (*const* lPtr)(ManagerInterface*, const uint8_t*, uint8_t*,
                              size_t*) =
        lhs.target<ipmi_ret_t (*)(ManagerInterface*, const uint8_t*, uint8_t*,
                                  size_t*)>();

    ipmi_ret_t (*const* rPtr)(ManagerInterface*, const uint8_t*, uint8_t*,
                              size_t*) =
        rhs.target<ipmi_ret_t (*)(ManagerInterface*, const uint8_t*, uint8_t*,
                                  size_t*)>();

    EXPECT_TRUE(lPtr);
    EXPECT_TRUE(rPtr);
    EXPECT_EQ(*lPtr, *rPtr);
    return;
}

} // namespace

TEST(ValidateBlobCommandTest, InvalidCommandReturnsFailure)
{
    // Verify we handle an invalid command.

    StrictMock<CrcMock> crc;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};

    request[0] = 0xff;         // There is no command 0xff.
    dataLen = sizeof(uint8_t); // There is no payload for CRC.
    ipmi_ret_t rc;

    EXPECT_EQ(nullptr,
              validateBlobCommand(&crc, request, reply, &dataLen, &rc));
    EXPECT_EQ(IPMI_CC_INVALID_FIELD_REQUEST, rc);
}

TEST(ValidateBlobCommandTest, ValidCommandWithoutPayload)
{
    // Verify we handle a valid command that doesn't have a payload.

    StrictMock<CrcMock> crc;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};

    request[0] = BlobOEMCommands::bmcBlobGetCount;
    dataLen = sizeof(uint8_t); // There is no payload for CRC.
    ipmi_ret_t rc;

    IpmiBlobHandler res =
        validateBlobCommand(&crc, request, reply, &dataLen, &rc);
    EXPECT_FALSE(res == nullptr);
    EqualFunctions(getBlobCount, res);
}

TEST(ValidateBlobCommandTest, WithPayloadMinimumLengthIs3VerifyChecks)
{
    // Verify that if there's a payload, it's at least one command byte and
    // two bytes for the crc16 and then one data byte.

    StrictMock<CrcMock> crc;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};

    request[0] = BlobOEMCommands::bmcBlobGetCount;
    dataLen = sizeof(uint8_t) + sizeof(uint16_t);
    // There is a payload, but there are insufficient bytes.
    ipmi_ret_t rc;

    EXPECT_EQ(nullptr,
              validateBlobCommand(&crc, request, reply, &dataLen, &rc));
    EXPECT_EQ(IPMI_CC_REQ_DATA_LEN_INVALID, rc);
}

TEST(ValidateBlobCommandTest, WithPayloadAndInvalidCrc)
{
    // Verify that the CRC is checked, and failure is reported.

    StrictMock<CrcMock> crc;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};

    auto req = reinterpret_cast<struct BmcBlobWriteTx*>(request);
    req->cmd = BlobOEMCommands::bmcBlobWrite;
    req->crc = 0x34;
    req->sessionId = 0x54;
    req->offset = 0x100;

    uint8_t expectedBytes[2] = {0x66, 0x67};
    std::memcpy(req->data, &expectedBytes[0], sizeof(expectedBytes));

    dataLen = sizeof(struct BmcBlobWriteTx) + sizeof(expectedBytes);

    // skip over cmd and crc.
    size_t expectedLen = dataLen - 3;

    EXPECT_CALL(crc, clear());
    EXPECT_CALL(crc, compute(_, expectedLen))
        .WillOnce(Invoke([&](const uint8_t* bytes, uint32_t length) {
            EXPECT_EQ(0, std::memcmp(&request[3], bytes, length));
        }));
    EXPECT_CALL(crc, get()).WillOnce(Return(0x1234));

    ipmi_ret_t rc;

    EXPECT_EQ(nullptr,
              validateBlobCommand(&crc, request, reply, &dataLen, &rc));
    EXPECT_EQ(IPMI_CC_UNSPECIFIED_ERROR, rc);
}

TEST(ValidateBlobCommandTest, WithPayloadAndValidCrc)
{
    // Verify the CRC is checked and if it matches, return the handler.

    StrictMock<CrcMock> crc;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};

    auto req = reinterpret_cast<struct BmcBlobWriteTx*>(request);
    req->cmd = BlobOEMCommands::bmcBlobWrite;
    req->crc = 0x3412;
    req->sessionId = 0x54;
    req->offset = 0x100;

    uint8_t expectedBytes[2] = {0x66, 0x67};
    std::memcpy(req->data, &expectedBytes[0], sizeof(expectedBytes));

    dataLen = sizeof(struct BmcBlobWriteTx) + sizeof(expectedBytes);

    // skip over cmd and crc.
    size_t expectedLen = dataLen - 3;

    EXPECT_CALL(crc, clear());
    EXPECT_CALL(crc, compute(_, expectedLen))
        .WillOnce(Invoke([&](const uint8_t* bytes, uint32_t length) {
            EXPECT_EQ(0, std::memcmp(&request[3], bytes, length));
        }));
    EXPECT_CALL(crc, get()).WillOnce(Return(0x3412));

    ipmi_ret_t rc;

    IpmiBlobHandler res =
        validateBlobCommand(&crc, request, reply, &dataLen, &rc);
    EXPECT_FALSE(res == nullptr);
    EqualFunctions(writeBlob, res);
}

TEST(ValidateBlobCommandTest, InputIntegrationTest)
{
    // Given a request buffer generated by the host-side utility, verify it is
    // properly routed.

    Crc16 crc;
    size_t dataLen;
    uint8_t request[] = {0x02, 0x88, 0x21, 0x03, 0x00, 0x2f, 0x64, 0x65, 0x76,
                         0x2f, 0x68, 0x61, 0x76, 0x65, 0x6e, 0x2f, 0x63, 0x6f,
                         0x6d, 0x6d, 0x61, 0x6e, 0x64, 0x5f, 0x70, 0x61, 0x73,
                         0x73, 0x74, 0x68, 0x72, 0x75, 0x00};

    // The above request to open a file for reading & writing named:
    // "/dev/haven/command_passthru"

    uint8_t reply[MAX_IPMI_BUFFER] = {0};

    dataLen = sizeof(request);
    ipmi_ret_t rc;

    IpmiBlobHandler res =
        validateBlobCommand(&crc, request, reply, &dataLen, &rc);
    EXPECT_FALSE(res == nullptr);
    EqualFunctions(openBlob, res);
}

TEST(ProcessBlobCommandTest, CommandReturnsNotOk)
{
    // Verify that if the IPMI command handler returns not OK that this is
    // noticed and returned.

    StrictMock<CrcMock> crc;
    StrictMock<ManagerMock> manager;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};

    IpmiBlobHandler h = [](ManagerInterface* mgr, const uint8_t* reqBuf,
                           uint8_t* replyCmdBuf,
                           size_t* dataLen) { return IPMI_CC_INVALID; };

    dataLen = sizeof(request);

    EXPECT_EQ(IPMI_CC_INVALID,
              processBlobCommand(h, &manager, &crc, request, reply, &dataLen));
}

TEST(ProcessBlobCommandTest, CommandReturnsOkWithNoPayload)
{
    // Verify that if the IPMI command handler returns OK but without a payload
    // it doesn't try to compute a CRC.

    StrictMock<CrcMock> crc;
    StrictMock<ManagerMock> manager;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};

    IpmiBlobHandler h = [](ManagerInterface* mgr, const uint8_t* reqBuf,
                           uint8_t* replyCmdBuf, size_t* dataLen) {
        (*dataLen) = 0;
        return IPMI_CC_OK;
    };

    dataLen = sizeof(request);

    EXPECT_EQ(IPMI_CC_OK,
              processBlobCommand(h, &manager, &crc, request, reply, &dataLen));
}

TEST(ProcessBlobCommandTest, CommandReturnsOkWithInvalidPayloadLength)
{
    // There is a minimum payload length of 2 bytes (the CRC only, no data, for
    // read), this returns 1.

    StrictMock<CrcMock> crc;
    StrictMock<ManagerMock> manager;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};

    IpmiBlobHandler h = [](ManagerInterface* mgr, const uint8_t* reqBuf,
                           uint8_t* replyCmdBuf, size_t* dataLen) {
        (*dataLen) = sizeof(uint8_t);
        return IPMI_CC_OK;
    };

    dataLen = sizeof(request);

    EXPECT_EQ(IPMI_CC_UNSPECIFIED_ERROR,
              processBlobCommand(h, &manager, &crc, request, reply, &dataLen));
}

TEST(ProcessBlobCommandTest, CommandReturnsOkWithValidPayloadLength)
{
    // There is a minimum payload length of 3 bytes, this command returns a
    // payload of 3 bytes and the crc code is called to process the payload.

    StrictMock<CrcMock> crc;
    StrictMock<ManagerMock> manager;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    uint32_t payloadLen = sizeof(uint16_t) + sizeof(uint8_t);

    IpmiBlobHandler h = [payloadLen](ManagerInterface* mgr,
                                     const uint8_t* reqBuf,
                                     uint8_t* replyCmdBuf, size_t* dataLen) {
        (*dataLen) = payloadLen;
        replyCmdBuf[2] = 0x56;
        return IPMI_CC_OK;
    };

    dataLen = sizeof(request);

    EXPECT_CALL(crc, clear());
    EXPECT_CALL(crc, compute(_, payloadLen - sizeof(uint16_t)));
    EXPECT_CALL(crc, get()).WillOnce(Return(0x3412));

    EXPECT_EQ(IPMI_CC_OK,
              processBlobCommand(h, &manager, &crc, request, reply, &dataLen));
    EXPECT_EQ(dataLen, payloadLen);

    uint8_t expectedBytes[3] = {0x12, 0x34, 0x56};
    EXPECT_EQ(0, std::memcmp(expectedBytes, reply, sizeof(expectedBytes)));
}
} // namespace blobs
