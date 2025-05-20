#include "real_filesystem.h"

#include <boost/filesystem.hpp>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "tagged_exceptions.h"

using FileMutator = std::function<void(boost::filesystem::path)>;
using FileMapRef = std::vector<std::pair<std::string, std::string>>;

// class fixture that creates files and facilitates addition of new ones
// to provide real files for the test cases.
class RealFilesystemTest : public ::testing::Test {
   private:
    boost::filesystem::path tempDir = {};
    boost::filesystem::path baseDir = {};
    std::string oobFile = "hackerman.md";
    FileMapRef _files;

   protected:
    void SetUp() override {
        namespace fs = boost::filesystem;
        tempDir = fs::temp_directory_path() / fs::unique_path();
        baseDir = tempDir / "inventory";
        fs::create_directories(baseDir);
    }

    void TearDown() override {
        namespace fs = boost::filesystem;
        fs::remove_all(tempDir);
    }

    std::string getTempDir() {
        return baseDir.string();
    }

    void createDir(std::string dirname) {
        namespace fs = boost::filesystem;
        fs::create_directories(baseDir / dirname);
    }
};

TEST_F(RealFilesystemTest, ExistsReturnsCorrectStatus) {
    RealFilesystem fs(getTempDir());
    fs.write("Music", "10", "{}");

    EXPECT_TRUE(fs.exists("Music", "10"));
    EXPECT_FALSE(fs.exists("Music", "99"));
    EXPECT_FALSE(fs.exists("Books", "10"));
}

TEST_F(RealFilesystemTest, WriteAndReadValidEntity) {
    RealFilesystem fs(getTempDir());
    fs.write("Shoes", "1", R"({"brand":"Nike"})");
    std::string result = fs.read("Shoes", "1");

    EXPECT_EQ(result, R"({"brand":"Nike"})");
}

TEST_F(RealFilesystemTest, ReadMissingThrowsNotFound) {
    RealFilesystem fs(getTempDir());
    EXPECT_THROW(fs.read("Books", "42"), expt::not_found_exception);
}

TEST_F(RealFilesystemTest, RemoveExistingEntrySucceeds) {
    RealFilesystem fs(getTempDir());
    fs.write("Books", "2", R"({"title":"1984"})");

    EXPECT_TRUE(fs.exists("Books", "2"));

    fs.remove("Books", "2");

    EXPECT_FALSE(fs.exists("Books", "2"));
}

TEST_F(RealFilesystemTest, RemoveMissingThrowsNotFound) {
    RealFilesystem fs(getTempDir());
    EXPECT_THROW(fs.remove("Movies", "7"), expt::not_found_exception);
}

TEST_F(RealFilesystemTest, CheckValidIdDoesNotThrow) {
    RealFilesystem fs(getTempDir());
    EXPECT_NO_THROW(fs.write("F1", "1", R"({"driver":"Max"})"));
}

TEST_F(RealFilesystemTest, CheckInvalidIdThrows) {
    RealFilesystem fs(getTempDir());
    EXPECT_THROW(fs.write("F1", "-18", R"({"driver":"Lance"})"), expt::invalid_id_exception);
    EXPECT_THROW(fs.write("F1", "norris", R"({"driver":"Lando"})"), expt::invalid_id_exception);
}

TEST_F(RealFilesystemTest, ListIdsReturnsCorrectValues) {
    RealFilesystem fs(getTempDir());
    fs.write("Shoes", "1", "{}");
    fs.write("Shoes", "2", "{}");

    std::vector<std::string> ids = fs.list_ids("Shoes");
    EXPECT_EQ(ids.size(), 2);
    EXPECT_NE(std::find(ids.begin(), ids.end(), "1"), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), "2"), ids.end());
}

TEST_F(RealFilesystemTest, ListIdsReturnsEmptyForMissingDir) {
    RealFilesystem fs(getTempDir());
    std::vector<std::string> ids = fs.list_ids("NoShoes");
    EXPECT_TRUE(ids.empty());
}

TEST_F(RealFilesystemTest, NextIdReturnsIncrementedMaxId) {
    RealFilesystem fs(getTempDir());
    fs.write("Cars", "3", "{}");
    fs.write("Cars", "6", "{}");

    std::string next = fs.next_id("Cars");
    EXPECT_EQ(next, "7");
}

TEST_F(RealFilesystemTest, WriteFailsToOpen) {
    RealFilesystem fs(getTempDir());
    createDir("fake/7");
    EXPECT_THROW(fs.write("fake", "7", "{}"), expt::file_io_exception);
    // this trick does not work for read because of POSIX stuff
}
