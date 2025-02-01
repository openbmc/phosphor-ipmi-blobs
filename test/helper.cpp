#include "helper.hpp"

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
    ipmi::RspType<std::vector<uint8_t>> reply, bool hasData)
{
    // Reply is in the form of
    // std::tuple<ipmi::Cc, std::optional<std::tuple<RetTypes...>>>
    EXPECT_EQ(::ipmi::ccSuccess, std::get<0>(reply));

    auto actualReply = std::get<1>(reply);
    EXPECT_TRUE(actualReply.has_value());
    if (!actualReply.has_value())
        return std::vector<uint8_t>{};

    auto data = std::get<0>(*actualReply);
    EXPECT_EQ(hasData, !data.empty());

    return hasData ? data : std::vector<uint8_t>{};
}

} // namespace blobs
