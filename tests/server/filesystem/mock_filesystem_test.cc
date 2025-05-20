#include "mock_filesystem.h"

#include "gtest/gtest.h"
#include "tagged_exceptions.h"

class MockFilesystemTest : public testing::Test {
   protected:
    MockFilesystem fs;

    void SetUp() override {
        fs.reset();
    }
};

TEST_F(MockFilesystemTest, ExistsReturnsCorrectStatus) {
    fs.write("Music", "10", "{}");

    EXPECT_TRUE(fs.exists("Music", "10"));
    EXPECT_FALSE(fs.exists("Music", "99"));
    EXPECT_FALSE(fs.exists("Books", "10"));
}

TEST_F(MockFilesystemTest, WriteAndReadValidEntity) {
    fs.write("Shoes", "1", R"({"brand":"Nike"})");
    std::string result = fs.read("Shoes", "1");

    EXPECT_EQ(result, R"({"brand":"Nike"})");
}

TEST_F(MockFilesystemTest, ReadMissingThrowsNotFound) {
    EXPECT_THROW(fs.read("Books", "42"), expt::not_found_exception);
}

TEST_F(MockFilesystemTest, RemoveExistingEntrySucceeds) {
    fs.write("Books", "2", R"({"title":"1984"})");

    EXPECT_TRUE(fs.exists("Books", "2"));

    fs.remove("Books", "2");

    EXPECT_FALSE(fs.exists("Books", "2"));
}

TEST_F(MockFilesystemTest, RemoveMissingThrowsNotFound) {
    EXPECT_THROW(fs.remove("Movies", "7"), expt::not_found_exception);
}

TEST_F(MockFilesystemTest, CheckValidIdDoesNotThrow) {
    EXPECT_NO_THROW(fs.write("F1", "1", R"({"driver":"Max"})"));
}

TEST_F(MockFilesystemTest, CheckInvalidIdThrows) {
    EXPECT_THROW(fs.write("F1", "-18", R"({"driver":"Lance"})"), expt::invalid_id_exception);
    EXPECT_THROW(fs.write("F1", "norris", R"({"driver":"Lando"})"), expt::invalid_id_exception);
}

TEST_F(MockFilesystemTest, ListIdsReturnsCorrectValues) {
    fs.write("Shoes", "1", "{}");
    fs.write("Shoes", "2", "{}");

    std::vector<std::string> ids = fs.list_ids("Shoes");
    EXPECT_EQ(ids.size(), 2);
    EXPECT_NE(std::find(ids.begin(), ids.end(), "1"), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), "2"), ids.end());
}

TEST_F(MockFilesystemTest, NextIdReturnsIncrementedMaxId) {
    fs.write("Cars", "3", "{}");
    fs.write("Cars", "6", "{}");

    std::string next = fs.next_id("Cars");
    EXPECT_EQ(next, "7");
}
