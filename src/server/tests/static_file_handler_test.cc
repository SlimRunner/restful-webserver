#include "../request_handlers/static_file_handler.h"

#include <boost/filesystem.hpp>
#include <fstream>
#include <functional>
#include <map>
#include <string>

#include "gtest/gtest.h"

using FileMutator = std::function<void(boost::filesystem::path)>;

// class fixture that creates files and facilitates addition of new ones
// to provide real files for the test cases. Use sparingly as each test
// case creates this entire fleet of files.
class StaticFileHandlerText : public ::testing::Test {
   private:
    std::map<std::string, std::string> files = {
        {"index.html", "<h1>Hello, world!</h1>"},
        {"style.css", "* { color: red }"},
        {"script.js", "application/javascript"},
        {"image.jpg", "mock of bin data for JPEG"},
        {"image.jpeg", "mock of bin data for JPEG"},
        {"image.png", "mock of bin data for PNG"},
        {"image.gif", "mock of bin data for GIF"},
        {"image.svg", "<svg fill=\"none\"></svg>"},
        {"text.txt", "foobar hello world"},
        {"doc.pdf", "mock of bin data for PDF"},
        {"arch.zip", "mock of bin data for ZIP"},
        {"schema.json", "{ a: {b: [1,2,3]}}"},
        {"schema.xml", "<?xml version=\"1.0\" encoding=\"UTF-8\"?> <a v=\"asd\">text</a>"},
        {"favicon.ico", "mock of bin data for ICO"},
        {"extensionless", "file without extension"},
        {"weird.zzy", "non supported extension"},
    };

    boost::filesystem::path tempDir;
    boost::filesystem::path baseDir;
    const std::string BASE_TAG = "/base";
    const std::string oobFile = "hackerman.md";

   protected:
    void SetUp() override {
        namespace fs = boost::filesystem;
        tempDir = fs::temp_directory_path() / fs::unique_path();
        baseDir = tempDir / "static";
        fs::create_directories(baseDir);

        for (auto const &file : files) {
            std::ofstream(baseDir / file.first) << file.second;
        }
        std::ofstream(tempDir / oobFile) << "get pwned";
    }

    void TearDown() override {
        namespace fs = boost::filesystem;
        fs::remove_all(tempDir);
    }

    StaticFileHandler getHandler() {
        return StaticFileHandler(BASE_TAG, baseDir.string());
    }

    HttpRequest getRequest(std::string method, std::string filename) {
        HttpRequest request;
        request.version = "HTTP/1.1";
        request.method = method;
        request.path = BASE_TAG + "/" + filename;
        request.headers = {};
        request.body = "";
        return request;
    }

    void addFile(std::string filename, std::string content, FileMutator mutator) {
        const auto filepath = baseDir / filename;
        std::ofstream(filepath) << content;
        namespace fs = boost::filesystem;
        mutator(filepath);
    }

    std::string getOutOfBoundsFilename() {
        // --
        return oobFile;
    }

    const std::map<std::string, std::string> &getMockFiles() {
        // --
        return files;
    }
};

// test that CanHandle successfully accepts correct paths
TEST(StaticFileHandlerTest, CanHandleValidPaths) {
    StaticFileHandler handler("/static", "./base/dir");
    EXPECT_TRUE(handler.CanHandle("/static/a/b/c"));
    EXPECT_TRUE(handler.CanHandle("/static/x/y"));
    EXPECT_TRUE(handler.CanHandle("/static/x/c"));
    EXPECT_TRUE(handler.CanHandle("/static"));
    EXPECT_TRUE(handler.CanHandle("/static/base/dir"));

    // this assertion is assumed in later tests because the
    // Content-Disposition code assumes this is possible
    handler = StaticFileHandler("", "./base/dir");
    EXPECT_TRUE(handler.CanHandle("somefile.txt"));
}

// test that CanHandle successfully rejects incorrect paths
TEST(StaticFileHandlerTest, CanHandleInvalidPaths) {
    StaticFileHandler handler("/static", "./base/dir");
    EXPECT_FALSE(handler.CanHandle("/foobar/a/b/c"));
    EXPECT_FALSE(handler.CanHandle("static/a/b/c"));
    EXPECT_FALSE(handler.CanHandle("~/static/a/b/c"));
    EXPECT_FALSE(handler.CanHandle("./base/dir/static/a/b/c"));
}

// test files that use all of the MimeTypes and verify that it can read
// them
TEST_F(StaticFileHandlerText, HandleRequestWithAllFiles) {
    auto handler = getHandler();
    for (const auto &file : getMockFiles()) {
        auto response = handler.HandleRequest(getRequest("GET", file.first));
        EXPECT_EQ(response.status, StatusCode::OK);
        EXPECT_EQ(response.body, file.second);
    }
}

// test that non-existent file returns 404
TEST_F(StaticFileHandlerText, HandleRequestWithNonExistentFile) {
    auto handler = getHandler();
    auto response = handler.HandleRequest(getRequest("GET", "non-existent.txt"));
    EXPECT_EQ(response.status, StatusCode::NOT_FOUND);
    EXPECT_EQ(response.body, "404 Not Found");
}

// test that providing incorrect method results in BAD_REQUEST
TEST_F(StaticFileHandlerText, HandleRequestWithBadRequest) {
    auto handler = getHandler();
    const auto iterFile = getMockFiles().begin();

    // make sure the fixture has at least one test
    EXPECT_TRUE(iterFile != getMockFiles().end());

    auto response = handler.HandleRequest(getRequest("POST", iterFile->first));
    EXPECT_EQ(response.status, StatusCode::BAD_REQUEST);
    EXPECT_EQ(response.body, "");
}

// tests that the request doesn't access files outside of the mapped
// base directory
TEST_F(StaticFileHandlerText, HandleRequestAttemptBacktracking) {
    auto handler = getHandler();
    auto request = getRequest("GET", "../" + getOutOfBoundsFilename());
    auto response = handler.HandleRequest(request);
    EXPECT_EQ(response.status, StatusCode::FORBIDDEN);
    EXPECT_EQ(response.body, "403 Forbidden");
}

// test that attempting to read unreadable file returns 404
TEST_F(StaticFileHandlerText, HandleRequestWithUnreadableFile) {
    namespace fs = boost::filesystem;
    auto handler = getHandler();
    constexpr auto filename = "secret.txt";

    // call fixture function to create file with unset read bit
    addFile(filename, "unreadable", [](fs::path fp) {
        boost::system::error_code ec;
        fs::permissions(fp, fs::owner_all & ~fs::owner_read, ec);
        if (ec) {
            GTEST_SKIP() << "Skipping test because permissions could not be changed: "
                         << ec.message();
        }
    });

    auto response = handler.HandleRequest(getRequest("GET", filename));
    EXPECT_EQ(response.status, StatusCode::NOT_FOUND);
    EXPECT_EQ(response.body, "404 Not Found");
}

// test that the function gracefully fails with 1000+ MB file
TEST_F(StaticFileHandlerText, HandleRequestFailureWithLargeFile) {
    namespace fs = boost::filesystem;
    auto handler = getHandler();
    constexpr auto filename = "massive_file.txt";
    constexpr std::size_t oversize = 100 * 1024 * 1024 + 1;
    const auto content = std::string(oversize, ' ');

    // call fixture function to create file with unset read bit
    addFile(filename, content, [](fs::path) {});

    auto response = handler.HandleRequest(getRequest("GET", filename));
    EXPECT_EQ(response.status, StatusCode::NOT_FOUND);
    EXPECT_EQ(response.body, "404 Not Found");
}

TEST(StaticFileHandlerTest, CanReadFromEmptyBaseTag) {
    // this cannot use the StaticFileHandlerText fixture because it
    // enforces /base as the tag to use. It cannot be changed.

    namespace fs = boost::filesystem;
    constexpr auto filename = "somefile.txt";
    constexpr auto filecontent = "foo bar";

    fs::path tempDir = fs::temp_directory_path() / fs::unique_path();
    fs::path baseDir = tempDir / "static";

    fs::create_directories(baseDir);

    // map temp directory to empty tag
    StaticFileHandler handler = StaticFileHandler("", baseDir.string());
    // create file
    std::ofstream(baseDir / filename) << filecontent;

    // construct request
    HttpRequest request;
    request.version = "HTTP/1.1";
    request.method = "GET";
    request.path = filename;
    request.headers = {};
    request.body = "";

    // should be read correctly
    auto response = handler.HandleRequest(request);
    EXPECT_EQ(response.status, StatusCode::OK);
    EXPECT_EQ(response.body, filecontent);
}
