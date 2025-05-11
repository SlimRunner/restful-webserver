#include "static_file_handler.h"

#include <boost/filesystem.hpp>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

using FileMutator = std::function<void(boost::filesystem::path)>;
using FileMapRef = std::vector<std::pair<std::string, std::string>>;

// class fixture that creates files and facilitates addition of new ones
// to provide real files for the test cases.
class StaticFileHandlerText : public ::testing::Test {
   private:
    boost::filesystem::path tempDir = {};
    boost::filesystem::path baseDir = {};
    std::string BASE_TAG = "/base";
    std::string oobFile = "hackerman.md";
    FileMapRef _files;

   protected:
    void SetUp() override {
        namespace fs = boost::filesystem;
        tempDir = fs::temp_directory_path() / fs::unique_path();
        baseDir = tempDir / "static";
        fs::create_directories(baseDir);
    }

    void TearDown() override {
        namespace fs = boost::filesystem;
        fs::remove_all(tempDir);
    }

    const FileMapRef &getFiles() {
        return _files;
    }

    const FileMapRef &createDefaultFiles() {
        _files.insert(
            _files.end(),
            {
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
            });
        for (auto const &file : _files) {
            std::ofstream(baseDir / file.first) << file.second;
        }
        return this->_files;
    }

    StaticFileHandler getHandler(std::string prefix) {
        return StaticFileHandler(prefix, {{"root", baseDir.string()}});
    }

    HttpRequest getRequest(std::string method, std::string filepath) {
        HttpRequest request;
        request.version = "HTTP/1.1";
        request.method = method;
        request.path = filepath;
        request.headers = {};
        request.body = "";
        return request;
    }

    const FileMapRef &addUnreachableFile(std::string filename, std::string content,
                                         FileMutator mutator) {
        const auto filepath = tempDir / filename;
        std::ofstream(filepath) << content;
        namespace fs = boost::filesystem;
        mutator(filepath);
        _files.push_back({filename, content});
        return this->_files;
    }

    const FileMapRef &addFile(std::string filename, std::string content, FileMutator mutator) {
        const auto filepath = baseDir / filename;
        std::ofstream(filepath) << content;
        namespace fs = boost::filesystem;
        mutator(filepath);
        _files.push_back({filename, content});
        return this->_files;
    }
};

// // test that CanHandle successfully accepts correct paths
// TEST(StaticFileHandlerTest, CanHandleValidPaths) {
//     StaticFileHandler handler("/static", "./base/dir");
//     EXPECT_TRUE(handler.CanHandle("/static/a/b/c"));
//     EXPECT_TRUE(handler.CanHandle("/static/x/y"));
//     EXPECT_TRUE(handler.CanHandle("/static/x/c"));
//     EXPECT_TRUE(handler.CanHandle("/static"));
//     EXPECT_TRUE(handler.CanHandle("/static/base/dir"));

//     // this assertion is assumed in later tests because the
//     // Content-Disposition code assumes this is possible
//     handler = StaticFileHandler("", "./base/dir");
//     EXPECT_TRUE(handler.CanHandle("somefile.txt"));
// }

// // test that CanHandle successfully rejects incorrect paths
// TEST(StaticFileHandlerTest, CanHandleInvalidPaths) {
//     StaticFileHandler handler("/static", "./base/dir");
//     EXPECT_FALSE(handler.CanHandle("/foobar/a/b/c"));
//     EXPECT_FALSE(handler.CanHandle("static/a/b/c"));
//     EXPECT_FALSE(handler.CanHandle("~/static/a/b/c"));
//     EXPECT_FALSE(handler.CanHandle("./base/dir/static/a/b/c"));
// }

// test files that use all of the MimeTypes and verify that it can read
// them
TEST_F(StaticFileHandlerText, HandleRequestWithAllFiles) {
    const std::string PREFIX = "/base";
    auto handler = getHandler(PREFIX);
    for (const auto &file : createDefaultFiles()) {
        std::shared_ptr<HttpResponse> response =
            handler.handle_request(getRequest("GET", PREFIX + "/" + file.first));
        EXPECT_EQ(response->status, StatusCode::OK);
        EXPECT_EQ(response->body, file.second);
    }
}

// test that non-existent file returns 404
TEST_F(StaticFileHandlerText, HandleRequestWithNonExistentFile) {
    const std::string PREFIX = "/base";
    const std::string FILEPATH = PREFIX + "/non-existent.txt";
    auto handler = getHandler(PREFIX);
    std::shared_ptr<HttpResponse> response = handler.handle_request(getRequest("GET", FILEPATH));
    EXPECT_EQ(response->status, StatusCode::NOT_FOUND);
    EXPECT_EQ(response->body, "404 Not Found");
}

// test that providing incorrect method results in BAD_REQUEST
TEST_F(StaticFileHandlerText, HandleRequestWithBadRequest) {
    const FileMapRef &files = createDefaultFiles();
    const std::string PREFIX = "/base";
    const std::string FILEPATH = PREFIX + "/" + files.at(0).first;
    auto handler = getHandler(PREFIX);

    // make sure the fixture has at least one test
    EXPECT_FALSE(files.empty());

    std::shared_ptr<HttpResponse> response = handler.handle_request(getRequest("POST", FILEPATH));
    EXPECT_EQ(response->status, StatusCode::BAD_REQUEST);
    EXPECT_EQ(response->body, "");
}

// tests that the request doesn't access files outside of the mapped
// base directory
TEST_F(StaticFileHandlerText, HandleRequestAttemptBacktracking) {
    const std::string PREFIX = "/base";
    const FileMapRef &files = addUnreachableFile("secret.txt", "get pwned", [](auto) {});
    const std::string FILEPATH = PREFIX + "/../" + files.at(0).first;
    auto handler = getHandler(PREFIX);

    auto request = getRequest("GET", FILEPATH);
    std::shared_ptr<HttpResponse> response = handler.handle_request(request);
    EXPECT_EQ(response->status, StatusCode::FORBIDDEN);
    EXPECT_EQ(response->body, "403 Forbidden");
}

// test that the function gracefully fails with 1000+ MB file
TEST_F(StaticFileHandlerText, HandleRequestFailureWithLargeFile) {
    constexpr std::size_t oversize = 100 * 1024 * 1024 + 1;
    const std::string PREFIX = "/base";
    const FileMapRef &files = addFile("massive_file.txt", std::string(oversize, ' '), [](auto) {});
    const std::string FILEPATH = PREFIX + "/" + files.at(0).first;
    auto handler = getHandler(PREFIX);

    std::shared_ptr<HttpResponse> response = handler.handle_request(getRequest("GET", FILEPATH));
    EXPECT_EQ(response->status, StatusCode::NOT_FOUND);
    EXPECT_EQ(response->body, "404 Not Found");
}

TEST_F(StaticFileHandlerText, CanReadFromEmptyBaseTag) {
    constexpr std::size_t oversize = 100 * 1024 * 1024 + 1;
    const std::string PREFIX = "/base";
    const FileMapRef &files = addFile("somefile.txt", "foo bar", [](auto) {});
    const std::string FILEPATH = PREFIX + "/" + files.at(0).first;
    const std::string CONTENT = files.at(0).second;
    auto handler = getHandler(PREFIX);

    // should be read correctly
    std::shared_ptr<HttpResponse> response = handler.handle_request(getRequest("GET", FILEPATH));
    EXPECT_EQ(response->status, StatusCode::OK);
    EXPECT_EQ(response->body, CONTENT);
}
