#pragma once

#include "ipmi.hpp"
#include "manager.hpp"

#include <ipmid/api.h>

#include <ipmid/api-types.hpp>

#include <functional>
#include <span>
#include <utility>
#include <vector>

namespace blobs
{

using IpmiBlobHandler =
    std::function<Resp(ManagerInterface* mgr, std::span<const uint8_t> data)>;

/**
 * Validate the IPMI request and determine routing.
 *
 * @param[in] cmd  Requested command
 * @param[in] data Requested data
 * @return the ipmi command handler, or nullopt on failure.
 */
IpmiBlobHandler validateBlobCommand(uint8_t cmd, std::span<const uint8_t> data);

/**
 * Call the IPMI command and process the result, including running the CRC
 * computation for the reply message if there is one.
 *
 * @param[in] cmd - a function pointer to the ipmi command to process.
 * @param[in] mgr - a pointer to the manager interface.
 * @param[in] data - Requested data.
 * @param[in,out] maxSize - Maximum ipmi reply size
 * @return the ipmi command result.
 */
Resp processBlobCommand(IpmiBlobHandler cmd, ManagerInterface* mgr,
                        std::span<const uint8_t> data, size_t maxSize);

/**
 * Given an IPMI command, request buffer, and reply buffer, validate the request
 * and call processBlobCommand.
 */
Resp handleBlobCommand(uint8_t cmd, std::vector<uint8_t> data, size_t maxSize);
} // namespace blobs
