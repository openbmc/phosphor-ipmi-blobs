#pragma once

#include "internal/sys.hpp"

#include <gmock/gmock.h>

namespace blobs
{
namespace internal
{

class InternalDlSysMock : public DlSysInterface
{
  public:
    virtual ~InternalDlSysMock() = default;

    MOCK_METHOD(const char*, dlerror, (), (const, override));
    MOCK_METHOD(void*, dlopen, (const char*, int), (const, override));
    MOCK_METHOD(void*, dlsym, (void*, const char*), (const, override));
};

} // namespace internal
} // namespace blobs
