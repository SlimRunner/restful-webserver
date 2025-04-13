#include "gtest/gtest.h"
#include "config_parser.h"

TEST(NginxConfigParserTest, SimpleConfig) {
  NginxConfigParser parser;
  NginxConfig out_config;
  
  // Using a Relative Path from the Source Directory
  std::string config_path = std::string(TEST_DATA_DIR) + "/example_config";
  bool success = parser.Parse(config_path.c_str(), &out_config);

  EXPECT_TRUE(success);
}
// Test an empty config file 
TEST(NginxConfigParserTest, EmptyConfig) {
  NginxConfigParser parser;
  NginxConfig out_config;

  std::istringstream config("");
  EXPECT_TRUE(parser.Parse(&config, &out_config));
  EXPECT_EQ(out_config.statements_.size(), 0);
}

// Parser correctly fails on invalid syntax (missing ';')
TEST(NginxConfigParserTest, MissingSemicolon) {
  NginxConfigParser parser;
  NginxConfig out_config;

  std::istringstream config("foo bar");
  EXPECT_FALSE(parser.Parse(&config, &out_config));
}

// Parser can parse a basic block.
TEST(NginxConfigParserTest, SimpleBlock) {
  NginxConfigParser parser;
  NginxConfig out_config;

  std::istringstream config("server { listen 80; }");
  EXPECT_TRUE(parser.Parse(&config, &out_config));
  EXPECT_EQ(out_config.statements_.size(), 1);
}

// Parser supports nested structures.
TEST(NginxConfigParserTest, NestedBlock) {
  NginxConfigParser parser;
  NginxConfig out_config;

  std::istringstream config(
    "server {\n"
    "  listen 80;\n"
    "  location / {\n"
    "    root /data;\n"
    "  }\n"
    "}"
  );

  EXPECT_TRUE(parser.Parse(&config, &out_config));
  EXPECT_EQ(out_config.statements_.size(), 1);
}

// Parser fails on unmatched {
TEST(NginxConfigParserTest, UnmatchedBraces) {
  NginxConfigParser parser;
  NginxConfig out_config;

  std::istringstream config("server { listen 80;");
  EXPECT_FALSE(parser.Parse(&config, &out_config));
}

// Parser ignores comments correctly
TEST(NginxConfigParserTest, IgnoresComments) {
  NginxConfigParser parser;
  NginxConfig out_config;

  std::istringstream config("# comment line\nlisten 80;");
  EXPECT_TRUE(parser.Parse(&config, &out_config));
  EXPECT_EQ(out_config.statements_.size(), 1);
}

// Ensure multiple directives on new lines are parsed
TEST(NginxConfigParserTest, MultipleStatements) {
  NginxConfigParser parser;
  NginxConfig out_config;

  std::istringstream config(
    "worker_processes 1;\n"
    "events {\n"
    "  worker_connections 1024;\n"
    "}"
  );

  EXPECT_TRUE(parser.Parse(&config, &out_config));
  EXPECT_EQ(out_config.statements_.size(), 2);
}

