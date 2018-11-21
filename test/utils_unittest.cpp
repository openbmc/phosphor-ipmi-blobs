#include "dlsys_mock.hpp"
#include "fs.hpp"
#include "utils.hpp"

#include <blobs-ipmid/test/blob_mock.hpp>
#include <blobs-ipmid/test/manager_mock.hpp>
#include <experimental/filesystem>
#include <memory>

#include <gtest/gtest.h>

namespace fs = std::experimental::filesystem;

namespace blobs
{
using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::StrictMock;

std::vector<std::string>* returnList;

std::vector<std::string> getLibraryList(const std::string& path,
                                        PathMatcher check)
{
    return (returnList) ? *returnList : std::vector<std::string>();
}

std::unique_ptr<GenericBlobInterface> factoryReturn;

std::unique_ptr<GenericBlobInterface> fakeFactory()
{
    return std::move(factoryReturn);
}

TEST(UtilLoadLibraryTest, NoFilesFound)
{
    /* Verify nothing special happens when there are no files found. */

    StrictMock<internal::InternalDlSysMock> dlsys;
    StrictMock<ManagerMock> manager;

    loadLibraries(&manager, "", &dlsys);
}

TEST(UtilLoadLibraryTest, OneFileFoundIsLibrary)
{
    /* Verify if it finds a library, and everything works, it'll regsiter it.
     */

    std::vector<std::string> files = {"this.fake"};
    returnList = &files;

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

// TODO: Add regex checker unit-tests.

} // namespace blobs
