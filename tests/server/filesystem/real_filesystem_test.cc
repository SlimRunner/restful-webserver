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
    std::unique_ptr<RealFilesystem> real_fs;

    void SetUp() override {
        namespace fs = boost::filesystem;
        tempDir = fs::temp_directory_path() / fs::unique_path();
        baseDir = tempDir / "inventory";
        fs::create_directories(baseDir);
        real_fs = std::make_unique<RealFilesystem>(baseDir.string());
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
    real_fs->write({getTempDir(), "Music"}, "10", "{}");

    EXPECT_TRUE(real_fs->exists({getTempDir(), "Music"}, "10"));
    EXPECT_FALSE(real_fs->exists({getTempDir(), "Music"}, "99"));
    EXPECT_FALSE(real_fs->exists({getTempDir(), "Books"}, "10"));
}

TEST_F(RealFilesystemTest, WriteAndReadValidEntity) {
    real_fs->write({getTempDir(), "Shoes"}, "1", R"({"brand":"Nike"})");
    std::string result = real_fs->read({getTempDir(), "Shoes"}, "1");

    EXPECT_EQ(result, R"({"brand":"Nike"})");
}

TEST_F(RealFilesystemTest, ReadMissingThrowsNotFound) {
    EXPECT_THROW(real_fs->read({getTempDir(), "Books"}, "42"), expt::not_found_exception);
}

TEST_F(RealFilesystemTest, RemoveExistingEntrySucceeds) {
    real_fs->write({getTempDir(), "Books"}, "2", R"({"title":"1984"})");

    EXPECT_TRUE(real_fs->exists({getTempDir(), "Books"}, "2"));

    real_fs->remove({getTempDir(), "Books"}, "2");

    EXPECT_FALSE(real_fs->exists({getTempDir(), "Books"}, "2"));
}

TEST_F(RealFilesystemTest, RemoveMissingThrowsNotFound) {
    EXPECT_THROW(real_fs->remove({getTempDir(), "Movies"}, "7"), expt::not_found_exception);
}

TEST_F(RealFilesystemTest, CheckValidIdDoesNotThrow) {
    EXPECT_NO_THROW(real_fs->write({getTempDir(), "F1"}, "1", R"({"driver":"Max"})"));
}

TEST_F(RealFilesystemTest, CheckInvalidIdThrows) {
    EXPECT_THROW(real_fs->write({getTempDir(), "F1"}, "-18", R"({"driver":"Lance"})"),
                 expt::invalid_id_exception);
    EXPECT_THROW(real_fs->write({getTempDir(), "F1"}, "norris", R"({"driver":"Lando"})"),
                 expt::invalid_id_exception);
}

TEST_F(RealFilesystemTest, ListIdsReturnsCorrectValues) {
    real_fs->write({getTempDir(), "Shoes"}, "1", "{}");
    real_fs->write({getTempDir(), "Shoes"}, "2", "{}");

    std::vector<std::string> ids = real_fs->list_ids({getTempDir(), "Shoes"});
    EXPECT_EQ(ids.size(), 2);
    EXPECT_NE(std::find(ids.begin(), ids.end(), "1"), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), "2"), ids.end());
}

TEST_F(RealFilesystemTest, ListIdsReturnsEmptyForMissingDir) {
    std::vector<std::string> ids = real_fs->list_ids({getTempDir(), "NoShoes"});
    EXPECT_TRUE(ids.empty());
}

TEST_F(RealFilesystemTest, NextIdReturnsIncrementedMaxId) {
    real_fs->write({getTempDir(), "Cars"}, "3", "{}");
    real_fs->write({getTempDir(), "Cars"}, "6", "{}");

    std::string next = real_fs->next_id({getTempDir(), "Cars"});
    EXPECT_EQ(next, "7");
}

TEST_F(RealFilesystemTest, NextIdUsesNumericSorting) {
    real_fs->write({getTempDir(), "Cars"}, "10", "{}");
    real_fs->write({getTempDir(), "Cars"}, "1", "{}");
    real_fs->write({getTempDir(), "Cars"}, "9", "{}");

    std::string next = real_fs->next_id({getTempDir(), "Cars"});
    EXPECT_EQ(next, "11");
}

TEST_F(RealFilesystemTest, WriteFailsToOpen) {
    createDir("fake/7");
    EXPECT_THROW(real_fs->write({getTempDir(), "fake"}, "7", "{}"), expt::file_io_exception);
    // this trick does not work for read because of POSIX stuff
}
