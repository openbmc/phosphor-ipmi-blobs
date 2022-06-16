#include "ipmi.hpp"

#include <cstring>

#include <gtest/gtest.h>

namespace blobs
{

// ipmid.hpp isn't installed where we can grab it and this value is per BMC
// SoC.
#define MAX_IPMI_BUFFER 64

TEST(StringInputTest, NullPointerInput)
{
    // The method should verify it did receive a non-null input pointer.
    EXPECT_STREQ("", stringFromBuffer({}).c_str());
}

TEST(StringInputTest, ZeroBytesInput)
{
    // Verify that if the input length is 0 that it'll return the empty string.
    const std::string request = "asdf";
    EXPECT_STREQ("", stringFromBuffer(
                         std::vector<uint8_t>(request.begin(), request.end()))
                         .c_str());
}

TEST(StringInputTest, NulTerminatorNotFound)
{
    // Verify that if there isn't a nul-terminator found in an otherwise valid
    // string, it'll return the emptry string.
    std::array<char, MAX_IPMI_BUFFER> request;
    std::memset(request.data(), 'a', sizeof(request));
    EXPECT_STREQ("", stringFromBuffer(
                         std::vector<uint8_t>(request.begin(), request.end()))
                         .c_str());
}

TEST(StringInputTest, NulTerminatorFound)
{
    // Verify that if it's provided a valid nul-terminated string, it'll
    // return it.
    std::string request = "asdf";
    request.push_back('\0');
    EXPECT_STREQ("asdf", stringFromBuffer(std::vector<uint8_t>(request.begin(),
                                                               request.end()))
                             .c_str());
}
} // namespace blobs
