#include "helper.hpp"
#include "ipmi.hpp"
#include "manager_mock.hpp"
#include "process.hpp"

#include <cstring>
#include <ipmiblob/test/crc_mock.hpp>
#include <span>

#include <gtest/gtest.h>

// ipmid.hpp isn't installed where we can grab it and this value is per BMC
// SoC.
#define MAX_IPMI_BUFFER 64

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Return;
using ::testing::StrictMock;

namespace ipmiblob
{
CrcInterface* crcIntf = nullptr;

std::uint16_t generateCrc(const std::vector<std::uint8_t>& data)
{
    return (crcIntf) ? crcIntf->generateCrc(data) : 0x00;
}
} // namespace ipmiblob

namespace blobs
{
namespace
{

void EqualFunctions(IpmiBlobHandler lhs, IpmiBlobHandler rhs)
{
    EXPECT_FALSE(lhs == nullptr);
    EXPECT_FALSE(rhs == nullptr);

    Resp (*const* lPtr)(ManagerInterface*, std::span<const uint8_t>) =
        lhs.target<Resp (*)(ManagerInterface*, std::span<const uint8_t>)>();

    Resp (*const* rPtr)(ManagerInterface*, std::span<const uint8_t>) =
        rhs.target<Resp (*)(ManagerInterface*, std::span<const uint8_t>)>();

    EXPECT_TRUE(lPtr);
    EXPECT_TRUE(rPtr);
    EXPECT_EQ(*lPtr, *rPtr);
}

} // namespace

class ValidateBlobCommandTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        ipmiblob::crcIntf = &crcMock;
    }

    ipmiblob::CrcMock crcMock;
};

TEST_F(ValidateBlobCommandTest, InvalidCommandReturnsFailure)
{
    // Verify we handle an invalid command.
    std::vector<uint8_t> request(MAX_IPMI_BUFFER - 1);
    // There is no command 0xff.
    IpmiBlobHandler handler = validateBlobCommand(0xff, request);
    EXPECT_EQ(ipmi::responseInvalidFieldRequest(), handler(nullptr, {}));
}

TEST_F(ValidateBlobCommandTest, ValidCommandWithoutPayload)
{
    // Verify we handle a valid command that doesn't have a payload.
    std::vector<uint8_t> request(MAX_IPMI_BUFFER - 1);
    IpmiBlobHandler handler = validateBlobCommand(
        static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobGetCount), request);
    EXPECT_FALSE(handler == nullptr);
    EqualFunctions(getBlobCount, handler);
}

TEST_F(ValidateBlobCommandTest, WithPayloadMinimumLengthIs3VerifyChecks)
{
    // Verify that if there's a payload, it's at least one command byte and
    // two bytes for the crc16 and then one data byte.

    std::vector<uint8_t> request(sizeof(uint16_t));
    // There is a payload, but there are insufficient bytes.

    IpmiBlobHandler handler = validateBlobCommand(
        static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobGetCount), request);
    EXPECT_EQ(ipmi::responseReqDataLenInvalid(), handler(nullptr, {}));
}

TEST_F(ValidateBlobCommandTest, WithPayloadAndInvalidCrc)
{
    // Verify that the CRC is checked, and failure is reported.
    std::vector<uint8_t> request;
    BmcBlobWriteTx req;
    req.crc = 0x34;
    req.sessionId = 0x54;
    req.offset = 0x100;

    std::array<uint8_t, 2> expectedBytes = {0x66, 0x67};
    request.resize(sizeof(struct BmcBlobWriteTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobWriteTx));
    request.insert(request.end(), expectedBytes.begin(), expectedBytes.end());

    // skip over cmd and crc.
    std::vector<uint8_t> bytes(request.begin() + sizeof(req.crc),
                               request.end());
    EXPECT_CALL(crcMock, generateCrc(Eq(bytes))).WillOnce(Return(0x1234));

    IpmiBlobHandler handler = validateBlobCommand(
        static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobWrite), request);
    EXPECT_EQ(ipmi::responseUnspecifiedError(), handler(nullptr, {}));
}

TEST_F(ValidateBlobCommandTest, WithPayloadAndValidCrc)
{
    // Verify the CRC is checked and if it matches, return the handler.
    std::vector<uint8_t> request;
    BmcBlobWriteTx req;
    req.crc = 0x3412;
    req.sessionId = 0x54;
    req.offset = 0x100;

    std::array<uint8_t, 2> expectedBytes = {0x66, 0x67};
    request.resize(sizeof(struct BmcBlobWriteTx));
    std::memcpy(request.data(), &req, sizeof(struct BmcBlobWriteTx));
    request.insert(request.end(), expectedBytes.begin(), expectedBytes.end());

    // skip over cmd and crc.
    std::vector<uint8_t> bytes(request.begin() + sizeof(req.crc),
                               request.end());
    EXPECT_CALL(crcMock, generateCrc(Eq(bytes))).WillOnce(Return(0x3412));

    IpmiBlobHandler handler = validateBlobCommand(
        static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobWrite), request);
    EXPECT_FALSE(handler == nullptr);
    EqualFunctions(writeBlob, handler);
}

class ProcessBlobCommandTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        ipmiblob::crcIntf = &crcMock;
    }

    ipmiblob::CrcMock crcMock;
};

TEST_F(ProcessBlobCommandTest, CommandReturnsNotOk)
{
    // Verify that if the IPMI command handler returns not OK that this is
    // noticed and returned.

    StrictMock<ManagerMock> manager;
    std::vector<uint8_t> request(MAX_IPMI_BUFFER - 1);

    IpmiBlobHandler h = [](ManagerInterface*, std::span<const uint8_t>) {
        return ipmi::responseInvalidCommand();
    };

    EXPECT_EQ(ipmi::responseInvalidCommand(),
              processBlobCommand(h, &manager, request));
}

TEST_F(ProcessBlobCommandTest, CommandReturnsOkWithNoPayload)
{
    // Verify that if the IPMI command handler returns OK but without a payload
    // it doesn't try to compute a CRC.

    StrictMock<ManagerMock> manager;
    std::vector<uint8_t> request(MAX_IPMI_BUFFER - 1);

    IpmiBlobHandler h = [](ManagerInterface*, std::span<const uint8_t>) {
        return ipmi::responseSuccess(std::vector<uint8_t>());
    };

    EXPECT_EQ(ipmi::responseSuccess(std::vector<uint8_t>()),
              processBlobCommand(h, &manager, request));
}

TEST_F(ProcessBlobCommandTest, CommandReturnsOkWithInvalidPayloadLength)
{
    // There is a minimum payload length of 2 bytes (the CRC only, no data, for
    // read), this returns 1.

    StrictMock<ManagerMock> manager;
    std::vector<uint8_t> request(MAX_IPMI_BUFFER - 1);

    IpmiBlobHandler h = [](ManagerInterface*, std::span<const uint8_t>) {
        return ipmi::responseSuccess(std::vector<uint8_t>(1));
    };

    EXPECT_EQ(ipmi::responseUnspecifiedError(),
              processBlobCommand(h, &manager, request));
}

TEST_F(ProcessBlobCommandTest, CommandReturnsOkWithValidPayloadLength)
{
    // There is a minimum payload length of 3 bytes, this command returns a
    // payload of 3 bytes and the crc code is called to process the payload.

    StrictMock<ManagerMock> manager;
    std::vector<uint8_t> request(MAX_IPMI_BUFFER - 1);
    uint32_t payloadLen = sizeof(uint16_t) + sizeof(uint8_t);

    IpmiBlobHandler h = [payloadLen](ManagerInterface*,
                                     std::span<const uint8_t>) {
        std::vector<uint8_t> output(payloadLen, 0);
        output[2] = 0x56;
        return ipmi::responseSuccess(output);
    };

    EXPECT_CALL(crcMock, generateCrc(_)).WillOnce(Return(0x3412));

    auto result = validateReply(processBlobCommand(h, &manager, request));

    EXPECT_EQ(result.size(), payloadLen);
    EXPECT_THAT(result, ElementsAre(0x12, 0x34, 0x56));
}

} // namespace blobs
