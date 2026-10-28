#include "blob_mock.hpp"
#include "dlsys_mock.hpp"
#include "fs.hpp"
#include "manager_mock.hpp"
#include "utils.hpp"

#include <filesystem>
#include <memory>
#include <stdexcept>
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

std::unique_ptr<GenericBlobInterface> throwingFactory()
{
    throw std::runtime_error("factory failed");
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
    /* Verify if it finds a library, and everything works, it'll register it.
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

TEST_F(UtilLoadLibraryTest, ThrowingOpenDoesNotStopLaterHandlers)
{
    /* Verify that the following libraries are still loaded if one of them
     * throws on dlopen. */

    returnList = {"throws.fake", "good.fake"};

    StrictMock<internal::InternalDlSysMock> dlsys;
    StrictMock<ManagerMock> manager;
    void* handle = reinterpret_cast<void*>(0x01);

    factoryReturn = std::make_unique<BlobMock>();

    EXPECT_CALL(dlsys, dlopen(StrEq("throws.fake"), _))
        .WillOnce(::testing::Throw(std::runtime_error("static init failed")));
    EXPECT_CALL(dlsys, dlopen(StrEq("good.fake"), _)).WillOnce(Return(handle));

    EXPECT_CALL(dlsys, dlerror()).Times(2).WillRepeatedly(Return(nullptr));

    EXPECT_CALL(dlsys, dlsym(handle, StrEq("createHandler")))
        .WillOnce(Return(reinterpret_cast<void*>(fakeFactory)));

    EXPECT_CALL(manager, registerHandler(_));

    loadLibraries(&manager, "", &dlsys);
}

TEST_F(UtilLoadLibraryTest, ThrowingFactoryDoesNotStopLaterHandlers)
{
    /* Verify a createHandler that throws doesn't stop later handlers. */

    returnList = {"throws.fake", "good.fake"};

    StrictMock<internal::InternalDlSysMock> dlsys;
    StrictMock<ManagerMock> manager;
    void* handle1 = reinterpret_cast<void*>(0x01);
    void* handle2 = reinterpret_cast<void*>(0x02);

    factoryReturn = std::make_unique<BlobMock>();

    EXPECT_CALL(dlsys, dlopen(StrEq("throws.fake"), _))
        .WillOnce(Return(handle1));
    EXPECT_CALL(dlsys, dlopen(StrEq("good.fake"), _)).WillOnce(Return(handle2));

    EXPECT_CALL(dlsys, dlerror()).WillRepeatedly(Return(nullptr));

    EXPECT_CALL(dlsys, dlsym(handle1, StrEq("createHandler")))
        .WillOnce(Return(reinterpret_cast<void*>(throwingFactory)));
    EXPECT_CALL(dlsys, dlsym(handle2, StrEq("createHandler")))
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
