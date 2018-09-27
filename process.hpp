#pragma once

#include "crc.hpp"

#include <host-ipmid/ipmid-api.h>

#include <blobs-ipmid/manager.hpp>
#include <functional>

namespace blobs
{

using IpmiBlobHandler =
    std::function<ipmi_ret_t(ManagerInterface* mgr, const uint8_t* reqBuf,
                             uint8_t* replyCmdBuf, size_t* dataLen)>;

/**
 * Validate the IPMI request and determine routing.
 *
 * @param[in] crc - a pointer to the crc interface.
 * @param[in] reqBuf - a pointer to the ipmi request packet buffer.
 * @param[in,out] replyCmdBuf - a pointer to the ipmi reply packet buffer.
 * @param[in,out] dataLen - initially the request length, set to reply length
 *                          on return.
 * @return the ipmi command handler.
 */
IpmiBlobHandler validateBlobCommand(CrcInterface* crc, const uint8_t* reqBuf,
                                    uint8_t* replyCmdBuf, size_t* dataLen);

/**
 * Call the IPMI command and process the result, including running the CRC
 * computation for the reply message if there is one.
 *
 * @param[in] cmd - a funtion pointer to the ipmi command to process.
 * @param[in] mgr - a pointer to the manager interface.
 * @param[in] crc - a pointer to the crc interface.
 * @param[in] reqBuf - a pointer to the ipmi request packet buffer.
 * @param[in,out] replyCmdBuf - a pointer to the ipmi reply packet buffer.
 * @param[in,out] dataLen - initially the request length, set to reply length
 *                          on return.
 * @return the ipmi command result.
 */
ipmi_ret_t processBlobCommand(IpmiBlobHandler cmd, ManagerInterface* mgr,
                              CrcInterface* crc, const uint8_t* reqBuf,
                              uint8_t* replyCmdBuf, size_t* dataLen);
} // namespace blobs
