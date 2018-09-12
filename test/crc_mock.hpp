#pragma once

#include "crc.hpp"

#include <gmock/gmock.h>

namespace blobs
{

class CrcMock : public CrcInterface
{
  public:
    virtual ~CrcMock() = default;

    MOCK_METHOD0(clear, void());
    MOCK_METHOD2(compute, void(const uint8_t*, uint32_t));
    MOCK_CONST_METHOD0(get, uint16_t());
};
} // namespace blobs
