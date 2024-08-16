#include <ipmid/api-types.hpp>

#include <optional>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace blobs
{
std::vector<std::uint8_t> validateReply(
    ipmi::RspType<std::vector<uint8_t>> reply, bool hasData = true);
} // namespace blobs
