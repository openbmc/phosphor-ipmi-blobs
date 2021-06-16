#include "blob_mock.hpp"
#include "dlsys_mock.hpp"
#include "fs.hpp"
#include "manager_mock.hpp"
#include "utils.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

namespace blobs
{
using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::StrictMock;

std::vector<std::string> returnList;

std::vector<std::string> getLibraryList(const std::string&, PathMatcher)
{
    return returnList;
}

std::unique_ptr<GenericBlobInterface> factoryReturn;

std::unique_ptr<GenericBlobInterface> fakeFactory()
{
    return std::move(factoryReturn);
}

class UtilLoadLibraryTest : public ::testing::Test
{
  protected:
    UtilLoadLibraryTest()
    {
        returnList = {};
    }
};

TEST_F(UtilLoadLibraryTest, NoFilesFound)
{
    /* Verify nothing special happens when there are no files found. */

    StrictMock<internal::InternalDlSysMock> dlsys;
    StrictMock<ManagerMock> manager;

    loadLibraries(&manager, "", &dlsys);
}

TEST_F(UtilLoadLibraryTest, OneFileFoundIsLibrary)
{
    /* Verify if it finds a library, and everything works, it'll regsiter it.
     */

    returnList = {"this.fake"};

    StrictMock<internal::InternalDlSysMock> dlsys;
    StrictMock<ManagerMock> manager;
    void* handle = reinterpret_cast<void*>(0x01);
    auto blobMock = std::make_unique<BlobMock>();

    factoryReturn = std::move(blobMock);

    EXPECT_CALL(dlsys, dlopen(_, _)).WillOnce(Return(handle));

    EXPECT_CALL(dlsys, dlerror()).Times(2).WillRepeatedly(Return(nullptr));

    EXPECT_CALL(dlsys, dlsym(handle, StrEq("createHandler")))
        .WillOnce(Return(reinterpret_cast<void*>(fakeFactory)));

    EXPECT_CALL(manager, registerHandler(_));

    loadLibraries(&manager, "", &dlsys);
}

TEST(UtilLibraryMatchTest, TestAll)
{
    struct LibraryMatch
    {
        std::string name;
        bool expectation;
    };

    std::vector<LibraryMatch> tests = {
        {"libblobcmds.0.0.1", false}, {"libblobcmds.0.0", false},
        {"libblobcmds.0", false},     {"libblobcmds.10", false},
        {"libblobcmds.a", false},     {"libcmds.so.so.0", true},
        {"libcmds.so.0", true},       {"libcmds.so", true},
        {"libcmds.so.0.0.10", true},  {"libblobs.so.1000", true}};

    for (const auto& test : tests)
    {
        EXPECT_EQ(test.expectation, matchBlobHandler(test.name));
    }
}

} // namespace blobs
