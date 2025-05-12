#include "startup_utils.h"

#include <array>
#include <boost/filesystem.hpp>
#include <fstream>
#include <sstream>
#include <string>

#include "gtest/gtest.h"

class StartupUtilsFixture : public ::testing::Test {
   protected:
    boost::filesystem::path tempDir;

    void SetUp() override {
        namespace fs = boost::filesystem;
        tempDir = fs::temp_directory_path() / fs::unique_path();
        fs::create_directories(tempDir);
        std::ofstream(tempDir / "test_config.cfg") << "my_simple_config = 1";
    }

    void TearDown() override {
        namespace fs = boost::filesystem;
        fs::remove_all(tempDir);
    }

    std::string create_file(std::string name, std::string content) {
        std::ofstream(tempDir / name) << content;
        return (tempDir / name).string();
    }
};

TEST(ParseArgumentTest, ValidArgument) {
    constexpr const std::array<const char*, 2> params = {"cwd", "mock string"};
    auto path = parse_arguments(params.size(), params.data());
    EXPECT_TRUE(path.has_value());
    EXPECT_EQ(path.value(), params.at(1));
}

TEST(ParseArgumentTest, TooFewArguments) {
    constexpr const std::array<const char*, 1> params = {"cwd"};
    auto path = parse_arguments(params.size(), params.data());
    EXPECT_FALSE(path.has_value());
}

TEST(ParseArgumentTest, TooManyArguments) {
    constexpr const std::array<const char*, 3> params = {"a", "b", "c"};
    auto path = parse_arguments(params.size(), params.data());
    EXPECT_FALSE(path.has_value());
}

TEST_F(StartupUtilsFixture, InvalidConfigFile) {
    namespace fs = boost::filesystem;
    std::stringstream conf_text;
    conf_text << "port 8080\n";
    auto filename = create_file("config.txt", conf_text.str());
    EXPECT_TRUE(fs::exists(filename));

    auto conf = parse_config(filename);
    ASSERT_FALSE(conf.has_value());
}

TEST_F(StartupUtilsFixture, MissingPort) {
    namespace fs = boost::filesystem;
    std::stringstream conf_text;
    conf_text << "prop value;\n";
    auto filename = create_file("config.txt", conf_text.str());
    EXPECT_TRUE(fs::exists(filename));

    auto conf = parse_config(filename);
    ASSERT_FALSE(conf.has_value());
}

TEST_F(StartupUtilsFixture, ValidPortMissingLocation) {
    namespace fs = boost::filesystem;
    std::stringstream conf_text;
    conf_text << "port 2222;\n";
    auto filename = create_file("config.txt", conf_text.str());
    EXPECT_TRUE(fs::exists(filename));

    auto conf = parse_config(filename);
    ASSERT_TRUE(conf.has_value());
    ASSERT_EQ(conf.value().port_number, 2222);
}

TEST_F(StartupUtilsFixture, ValidEchoHandler) {
    namespace fs = boost::filesystem;
    std::stringstream conf_text;
    conf_text << "port 8080;\n";
    conf_text << "location /echo EchoHandler {}";
    auto filename = create_file("config.txt", conf_text.str());
    EXPECT_TRUE(fs::exists(filename));

    auto conf = parse_config(filename);
    ASSERT_TRUE(conf.has_value());
    ASSERT_EQ(conf->port_number, 8080);
    ASSERT_EQ(conf->route_map.size(), 1);
}

TEST_F(StartupUtilsFixture, ValidStaticHandler) {
    namespace fs = boost::filesystem;
    std::stringstream conf_text;
    conf_text << "port 80;\n";
    conf_text << "location /static StaticHandler {\n";
    conf_text << "root ./asd/asd;\n";
    conf_text << "}\n";
    auto filename = create_file("config.txt", conf_text.str());
    EXPECT_TRUE(fs::exists(filename));

    auto conf = parse_config(filename);
    ASSERT_TRUE(conf.has_value());
    ASSERT_EQ(conf->port_number, 80);
    ASSERT_EQ(conf->route_map.size(), 1);
}

TEST_F(StartupUtilsFixture, DisallowedTrailingSlash) {
    namespace fs = boost::filesystem;
    std::stringstream conf_text;
    conf_text << "port 8080;\n";
    conf_text << "location /echo/ EchoHandler {}";
    auto filename = create_file("config.txt", conf_text.str());
    EXPECT_TRUE(fs::exists(filename));

    auto conf = parse_config(filename);
    ASSERT_FALSE(conf.has_value());
}

TEST_F(StartupUtilsFixture, HandlerWithBadSignature) {
    namespace fs = boost::filesystem;
    std::stringstream conf_text;
    conf_text << "port 8080;\n";
    conf_text << "location /echo {}";
    auto filename = create_file("config.txt", conf_text.str());
    EXPECT_TRUE(fs::exists(filename));

    auto conf = parse_config(filename);
    // this is not an error, so it defaults to EchoHandler
    ASSERT_TRUE(conf.has_value());
    // cannot check handler name
}

TEST_F(StartupUtilsFixture, DuplicateServingPath) {
    namespace fs = boost::filesystem;
    std::stringstream conf_text;
    conf_text << "port 8080;\n";
    conf_text << "location /echo EchoHandler {}\n";
    conf_text << "location /echo EchoHandler {}";
    auto filename = create_file("config.txt", conf_text.str());
    EXPECT_TRUE(fs::exists(filename));

    auto conf = parse_config(filename);
    ASSERT_FALSE(conf.has_value());
}

TEST_F(StartupUtilsFixture, BadStaticHandlerMissingRoot) {
    namespace fs = boost::filesystem;
    std::stringstream conf_text;
    conf_text << "port 8080;\n";
    conf_text << "location /echo StaticHandler { asd ./path; }";
    auto filename = create_file("config.txt", conf_text.str());
    EXPECT_TRUE(fs::exists(filename));

    auto conf = parse_config(filename);
    // this is not an error, so it defaults to EchoHandler
    ASSERT_TRUE(conf.has_value());
    // cannot check handler name
}

TEST_F(StartupUtilsFixture, BadStaticHandler) {
    namespace fs = boost::filesystem;
    std::stringstream conf_text;
    conf_text << "port 8080;\n";
    conf_text << "location /echo StaticHandler { root one two; }";
    auto filename = create_file("config.txt", conf_text.str());
    EXPECT_TRUE(fs::exists(filename));

    auto conf = parse_config(filename);
    // this is not an error, so it defaults to EchoHandler
    ASSERT_TRUE(conf.has_value());
    // cannot check handler name
}

TEST_F(StartupUtilsFixture, NonExistingHandler) {
    namespace fs = boost::filesystem;
    std::stringstream conf_text;
    conf_text << "port 8080;\n";
    conf_text << "location /echo SomeRandomHandler {}";
    auto filename = create_file("config.txt", conf_text.str());
    EXPECT_TRUE(fs::exists(filename));

    auto conf = parse_config(filename);
    // this is not an error, so it defaults to EchoHandler
    ASSERT_TRUE(conf.has_value());
    // cannot check handler name
}

TEST_F(StartupUtilsFixture, InvalidSingleQuotedHandler) {
    namespace fs = boost::filesystem;
    std::stringstream conf_text;
    conf_text << "port 8080;\n";
    conf_text << "location '/echo who' EchoHandler {}";
    auto filename = create_file("config.txt", conf_text.str());
    EXPECT_TRUE(fs::exists(filename));

    auto conf = parse_config(filename);
    ASSERT_FALSE(conf.has_value());
}

TEST_F(StartupUtilsFixture, InvalidDoubleQuotedHandler) {
    namespace fs = boost::filesystem;
    std::stringstream conf_text;
    conf_text << "port 8080;\n";
    conf_text << "location \"/echo who\" EchoHandler {}";
    auto filename = create_file("config.txt", conf_text.str());
    EXPECT_TRUE(fs::exists(filename));

    auto conf = parse_config(filename);
    ASSERT_FALSE(conf.has_value());
}
