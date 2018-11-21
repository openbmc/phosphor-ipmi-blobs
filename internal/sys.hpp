#pragma once

namespace blobs
{

namespace internal
{
/**
 * Interface to the dynamic library loader.
 */
class DlSysInterface {
 public:
  virtual ~DlSysInterface() = default;

  virtual const char* dlerror() = 0;
  virtual void* dlopen(const char *filename, int flags) = 0;
};

class DlSysImpl : public DlSysInterface {
  public:
   const char* dlerror() override;
   void* dlopen(const char *filename, int flags) override;
};

extern DlSysImpl dlsys_impl;

}
}
