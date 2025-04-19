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

//Testing the toString method
TEST(NginxConfigToStringTest, BasicToString) {
  NginxConfigStatement* stmt = new NginxConfigStatement;
  stmt->tokens_.push_back("listen");
  stmt->tokens_.push_back("80");

  NginxConfig config;
  config.statements_.emplace_back(stmt);

  std::string expected = "listen 80;\n";
  EXPECT_EQ(config.ToString(0), expected);
}

//toString method with nested blocks
TEST(NginxConfigToStringTest, ToStringWithChildBlock) {
  NginxConfigStatement* parent = new NginxConfigStatement;
  parent->tokens_.push_back("server");

  NginxConfig* child = new NginxConfig;
  NginxConfigStatement* child_stmt = new NginxConfigStatement;
  child_stmt->tokens_.push_back("listen");
  child_stmt->tokens_.push_back("8080");
  child->statements_.push_back(std::unique_ptr<NginxConfigStatement>(child_stmt));
  parent->child_block_.reset(child);

  NginxConfig config;
  config.statements_.emplace_back(parent);

  std::string str = config.ToString(0);
  EXPECT_NE(str.find("server {"), std::string::npos);
  EXPECT_NE(str.find("listen 8080;"), std::string::npos);
}

//Test that the parser correctly handles single-quoted strings with spaces
TEST(NginxConfigParserTest, HandlesSingleQuotedTokens) {
  NginxConfigParser parser;
  NginxConfig out_config;

  std::istringstream config("location '/path with spaces';");
  EXPECT_TRUE(parser.Parse(&config, &out_config));
}

//Tests that the parser returns false for unterminated single-quoted strings
TEST(NginxConfigParserTest, UnterminatedQuoteTriggersError) {
  NginxConfigParser parser;
  NginxConfig out_config;

  std::istringstream config("location 'unterminated_path;");
  EXPECT_FALSE(parser.Parse(&config, &out_config));  // Should return false
}

// Tests that the parser returns false when given a nonexistent file path
TEST(NginxConfigParserTest, FileOpenFailureReturnsFalse) {
  NginxConfigParser parser;
  NginxConfig out_config;

  EXPECT_FALSE(parser.Parse("nonexistent.conf", &out_config));
}

