#include "utils.hpp"

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace blobs
{

TEST(UtilsTest, VerifyFilesAreChecked)
{

    struct TestCase
    {
        std::string value;
        bool expected;
    };

    std::vector<TestCase> tests = {
        {"this.so", true},
        {"this.so.1.1.1", true},
        {"this.bin", false},
    };

    for (const auto& t : tests)
    {
        EXPECT_EQ(t.expected, handlerSelect(t.value));
    }
}

} // namespace blobs
