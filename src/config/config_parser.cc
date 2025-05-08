// An nginx config file parser.
//
// See:
//   http://wiki.nginx.org/Configuration
//   http://blog.martinfjordvald.com/2010/07/nginx-primer/
//
// How Nginx does it:
//   http://lxr.nginx.org/source/src/core/ngx_conf_file.c

#include "config_parser.h"

#include <boost/log/trivial.hpp>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <vector>

#include "logger.h"
// Default constructor for NginxConfigParser.
NginxConfigParser::NginxConfigParser() {}

/*
Converts the NginxConfig object into a formatted string.
Recursively calls ToString on each statement with indentation.
*/
std::string NginxConfig::ToString(int depth) {
    std::string serialized_config;
    for (const auto& statement : statements_) {
        serialized_config.append(statement->ToString(depth));
    }
    return serialized_config;
}

/*
Converts a single config statement to a formatted string.
Includes nested blocks if present, with proper indentation.
*/
std::string NginxConfigStatement::ToString(int depth) {
    std::string serialized_statement;
    for (int i = 0; i < depth; ++i) {
        serialized_statement.append("  ");
    }
    for (unsigned int i = 0; i < tokens_.size(); ++i) {
        if (i != 0) {
            serialized_statement.append(" ");
        }
        serialized_statement.append(tokens_[i]);
    }
    if (child_block_.get() != nullptr) {
        serialized_statement.append(" {\n");
        serialized_statement.append(child_block_->ToString(depth + 1));
        for (int i = 0; i < depth; ++i) {
            serialized_statement.append("  ");
        }
        serialized_statement.append("}");
    } else {
        serialized_statement.append(";");
    }
    serialized_statement.append("\n");
    return serialized_statement;
}

// Returns a human-readable string representation of a TokenType enum value.
const char* NginxConfigParser::TokenTypeAsString(TokenType type) {
    switch (type) {
        case TOKEN_TYPE_START:
            return "TOKEN_TYPE_START";
        case TOKEN_TYPE_NORMAL:
            return "TOKEN_TYPE_NORMAL";
        case TOKEN_TYPE_START_BLOCK:
            return "TOKEN_TYPE_START_BLOCK";
        case TOKEN_TYPE_END_BLOCK:
            return "TOKEN_TYPE_END_BLOCK";
        case TOKEN_TYPE_COMMENT:
            return "TOKEN_TYPE_COMMENT";
        case TOKEN_TYPE_STATEMENT_END:
            return "TOKEN_TYPE_STATEMENT_END";
        case TOKEN_TYPE_EOF:
            return "TOKEN_TYPE_EOF";
        case TOKEN_TYPE_ERROR:
            return "TOKEN_TYPE_ERROR";
        default:
            return "Unknown token type";
    }
}

/*
Extracts a single token from the input stream.
Supports quoted strings, comments, and special characters.
Returns the token type and writes the value into `*value`.
*/
NginxConfigParser::TokenType NginxConfigParser::ParseToken(std::istream* input,
                                                           std::string* value) {
    TokenParserState state = TOKEN_STATE_INITIAL_WHITESPACE;
    while (input->good()) {
        const char c = input->get();
        if (!input->good()) {
            break;
        }
        switch (state) {
            case TOKEN_STATE_INITIAL_WHITESPACE:
                switch (c) {
                    case '{':
                        *value = c;
                        return TOKEN_TYPE_START_BLOCK;
                    case '}':
                        *value = c;
                        return TOKEN_TYPE_END_BLOCK;
                    case '#':
                        *value = c;
                        state = TOKEN_STATE_TOKEN_TYPE_COMMENT;
                        continue;
                    case '"':
                        *value = c;
                        state = TOKEN_STATE_DOUBLE_QUOTE;
                        continue;
                    case '\'':
                        *value = c;
                        state = TOKEN_STATE_SINGLE_QUOTE;
                        continue;
                    case ';':
                        *value = c;
                        return TOKEN_TYPE_STATEMENT_END;
                    case ' ':
                    case '\t':
                    case '\n':
                    case '\r':
                        continue;
                    default:
                        *value += c;
                        state = TOKEN_STATE_TOKEN_TYPE_NORMAL;
                        continue;
                }
            case TOKEN_STATE_SINGLE_QUOTE:
                *value += c;
                if (c == '\'') {
                    return TOKEN_TYPE_NORMAL;
                }
                continue;
            case TOKEN_STATE_DOUBLE_QUOTE:
                *value += c;
                if (c == '"') {
                    return TOKEN_TYPE_NORMAL;
                }
                continue;
            case TOKEN_STATE_TOKEN_TYPE_COMMENT:
                if (c == '\n' || c == '\r') {
                    return TOKEN_TYPE_COMMENT;
                }
                *value += c;
                continue;
            case TOKEN_STATE_TOKEN_TYPE_NORMAL:
                if (c == ' ' || c == '\t' || c == '\n' || c == '\t' || c == ';' || c == '{' ||
                    c == '}') {
                    input->unget();
                    return TOKEN_TYPE_NORMAL;
                }
                *value += c;
                continue;
        }
    }

    // If we get here, we reached the end of the file.
    if (state == TOKEN_STATE_SINGLE_QUOTE || state == TOKEN_STATE_DOUBLE_QUOTE) {
        return TOKEN_TYPE_ERROR;
    }

    return TOKEN_TYPE_EOF;
}

/*
Parses an nginx-style config from a stream into a NginxConfig object.
Builds a hierarchical structure of statements and blocks.
Returns true if parsing is successful and the config is valid.
*/
bool NginxConfigParser::Parse(std::istream* config_file, NginxConfig* config) {
    std::stack<NginxConfig*> config_stack;
    config_stack.push(config);
    TokenType last_token_type = TOKEN_TYPE_START;
    TokenType token_type;
    while (true) {
        std::string token;
        token_type = ParseToken(config_file, &token);
        BOOST_LOG_TRIVIAL(debug) << TokenTypeAsString(token_type) << ": " << token.c_str();
        if (token_type == TOKEN_TYPE_ERROR) {
            break;
        }

        if (token_type == TOKEN_TYPE_COMMENT) {
            // Skip comments.
            continue;
        }

        if (token_type == TOKEN_TYPE_START) {
            // Error.
            break;
        } else if (token_type == TOKEN_TYPE_NORMAL) {
            if (last_token_type == TOKEN_TYPE_START ||
                last_token_type == TOKEN_TYPE_STATEMENT_END ||
                last_token_type == TOKEN_TYPE_START_BLOCK ||
                last_token_type == TOKEN_TYPE_END_BLOCK || last_token_type == TOKEN_TYPE_NORMAL) {
                if (last_token_type != TOKEN_TYPE_NORMAL) {
                    config_stack.top()->statements_.emplace_back(new NginxConfigStatement);
                }
                config_stack.top()->statements_.back().get()->tokens_.push_back(token);
            } else {
                // Error.
                break;
            }
        } else if (token_type == TOKEN_TYPE_STATEMENT_END) {
            if (last_token_type != TOKEN_TYPE_NORMAL) {
                // Error.
                break;
            }
        } else if (token_type == TOKEN_TYPE_START_BLOCK) {
            if (last_token_type != TOKEN_TYPE_NORMAL) {
                // Error.
                break;
            }
            NginxConfig* const new_config = new NginxConfig;
            config_stack.top()->statements_.back().get()->child_block_.reset(new_config);
            config_stack.push(new_config);
        } else if (token_type == TOKEN_TYPE_END_BLOCK) {
            if (last_token_type != TOKEN_TYPE_STATEMENT_END &&
                // allowing nested blocks. a '}' can follow another '}'
                last_token_type != TOKEN_TYPE_END_BLOCK &&
                last_token_type != TOKEN_TYPE_START_BLOCK) {
                break;
            }
            if (config_stack.size() == 1) {
                // Prevent popping root config
                break;
            }
            config_stack.pop();
        } else if (token_type == TOKEN_TYPE_EOF) {
            // if we reached EOF and last token was either a:
            // STATEMENT_END or END_BLOCK (i.e ; or }) or if the file
            // is empty, return true, if not the case, break.
            if (last_token_type == TOKEN_TYPE_STATEMENT_END ||
                last_token_type == TOKEN_TYPE_END_BLOCK || last_token_type == TOKEN_TYPE_START) {
                // ensure the stack is empty before returning true
                return config_stack.size() == 1;
            }
            break;
        } else {
            // Error. Unknown token.
            break;
        }
        last_token_type = token_type;
    }

    BOOST_LOG_TRIVIAL(error) << "Bad transition from " << TokenTypeAsString(last_token_type)
                             << " to " << TokenTypeAsString(token_type);
    return false;
}

/*
Parses an nginx-style config file by filename.
Opens the file, delegates to the stream-based Parse method.
Returns true if the config is valid.
*/
bool NginxConfigParser::Parse(const char* file_name, NginxConfig* config) {
    std::ifstream config_file;
    config_file.open(file_name);
    if (!config_file.good()) {
        BOOST_LOG_TRIVIAL(error) << "Failed to open config file: " << file_name;
        return false;
    }

    const bool return_value = Parse(dynamic_cast<std::istream*>(&config_file), config);
    config_file.close();
    return return_value;
}
