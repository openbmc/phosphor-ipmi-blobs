#include "crc.hpp"

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace blobs
{

TEST(Crc16Test, VerifyCrcValue)
{
    // Verify the crc16 is producing the value we expect.

    // Origin: security/crypta/ipmi/portable/ipmi_utils_test.cc
    struct CrcTestVector
    {
        std::string input;
        uint16_t output;
    };

    std::string longString =
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAA";

    std::vector<CrcTestVector> vectors({{"", 0x1D0F},
                                        {"A", 0x9479},
                                        {"123456789", 0xE5CC},
                                        {longString, 0xE938}});

    Crc16 crc;

    for (const CrcTestVector& testVector : vectors)
    {
        crc.clear();
        auto data = reinterpret_cast<const uint8_t*>(testVector.input.data());
        crc.compute(data, testVector.input.size());
        EXPECT_EQ(crc.get(), testVector.output);
    }
}
} // namespace blobs
