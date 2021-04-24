#pragma once

#include <blobs-ipmid/blobs.hpp>

#include <gmock/gmock.h>

namespace blobs
{

class BlobMock : public GenericBlobInterface
{
  public:
    virtual ~BlobMock() = default;

    MOCK_METHOD(bool, canHandleBlob, (const std::string&), (override));
    MOCK_METHOD(std::vector<std::string>, getBlobIds, (), (override));
    MOCK_METHOD(bool, deleteBlob, (const std::string&), (override));
    MOCK_METHOD(bool, stat, (const std::string&, BlobMeta*), (override));
    MOCK_METHOD(bool, open, (uint16_t, uint16_t, const std::string&),
                (override));
    MOCK_METHOD(std::vector<uint8_t>, read, (uint16_t, uint32_t, uint32_t),
                (override));
    MOCK_METHOD(bool, write, (uint16_t, uint32_t, const std::vector<uint8_t>&),
                (override));
    MOCK_METHOD(bool, writeMeta,
                (uint16_t, uint32_t, const std::vector<uint8_t>&), (override));
    MOCK_METHOD(bool, commit, (uint16_t, const std::vector<uint8_t>&),
                (override));
    MOCK_METHOD(bool, close, (uint16_t), (override));
    MOCK_METHOD(bool, stat, (uint16_t, BlobMeta*), (override));
    MOCK_METHOD(bool, expire, (uint16_t), (override));
};
} // namespace blobs
