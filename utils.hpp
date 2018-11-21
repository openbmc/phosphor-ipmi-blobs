#pragma once

#include "internal/sys.hpp"

#include <blobs-ipmid/manager.hpp>
#include <string>

namespace blobs
{

/**
 * @brief Given a path, find libraries (*.so only) and load them.
 *
 * @param[in] manager - pointer to a manager
 * @param[in] paths - list of fully qualified paths to libraries to load.
 * @param[in] sys - pointer to implementation of the dlsys interface.
 */
void loadLibraries(ManagerInterface* manager, const std::string& path,
                   const internal::DlSysInterface* sys = &internal::dlsys_impl);

} // namespace blobs
