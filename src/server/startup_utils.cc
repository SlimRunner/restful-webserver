#include "startup_utils.h"

#include <boost/log/trivial.hpp>
#include <set>

#include "logger.h"
#include "server.h"
#include "tagged_exceptions.h"

using nginx_shared_statement = std::shared_ptr<NginxConfigStatement>;
using leaf_config_map = std::map<std::string, std::vector<std::vector<std::string>>>;
using nested_config_map = std::map<std::string, std::vector<nginx_shared_statement>>;
using nginx_config_map = std::map<std::string, std::vector<nginx_shared_statement>>;

static nginx_config_map unroll_one_level(const std::vector<nginx_shared_statement> &);
static bool string_is_quoted(std::string);

std::optional<std::string> parse_arguments(int argc, const char *const argv[]) {
    if (argc != 2) {
        BOOST_LOG_TRIVIAL(error) << "Invalid usage: "
                                    "Expected a config file. Usage: webserver <config_file>";
        return {};
    }

    BOOST_LOG_TRIVIAL(info) << "cwd: " << argv[0];
    return std::string(argv[1]);
}

std::optional<config_payload> parse_config(std::string filepath) {
    constexpr auto PORT_KEY = "port";
    constexpr auto LOCATION_KEY = "location";

    NginxConfigParser config_parser;
    NginxConfig config;

    // NginxConfigParser does not support strings just c-strings
    if (!config_parser.Parse(filepath.c_str(), &config)) {
        BOOST_LOG_TRIVIAL(error) << "Nginx repored a parsing error: " << filepath;
        return {};
    }
    BOOST_LOG_TRIVIAL(info) << "File parsed by Nginx successfully";

    nginx_config_map root_config_map = unroll_one_level(config.statements_);

    if (!root_config_map.count(PORT_KEY)) {
        BOOST_LOG_TRIVIAL(error) << "port was not found in config file: " << filepath;
        return {};
    }
    BOOST_LOG_TRIVIAL(info) << "Port found in config";

    config_payload payload = {};
    // this line implicitly chooses the first "port" key found
    payload.port_number = std::stoi(root_config_map.at(PORT_KEY).at(0)->tokens_.at(1));

    if (!root_config_map.count(LOCATION_KEY)) {
        BOOST_LOG_TRIVIAL(warning) << "No handlers configured. Defaulting to EchoHandler.";
        payload.handlers.push_back(std::make_shared<EchoHandler>("/"));
        return payload;
    }

    // this is used to validate serving path uniqueness
    std::set<std::string> serving_path_set;

    for (auto const &loc_items : root_config_map.at(LOCATION_KEY)) {
        const auto &tokens = loc_items->tokens_;
        const auto &child_block = loc_items->child_block_;

        if (tokens.size() != 3) {
            BOOST_LOG_TRIVIAL(warning) << "Skipping handler with bad signature.";
            BOOST_LOG_TRIVIAL(info) << loc_items->ToString(0);
            continue;
        }

        const auto &serving_path = tokens.at(1);
        const auto &handler_name = tokens.at(2);
        BOOST_LOG_TRIVIAL(info) << "Handler found in config: " << handler_name;

        if (serving_path_set.count(serving_path)) {
            BOOST_LOG_TRIVIAL(error) << "Duplicate serving path found: " << serving_path;
            return {};
        }

        if (!serving_path.empty() && serving_path.back() == '/') {
            BOOST_LOG_TRIVIAL(error) << "Trailing slash found in serving path: " << serving_path;
            return {};
        }

        if (string_is_quoted(serving_path)) {
            BOOST_LOG_TRIVIAL(error) << "Quotes are not allowed: " << serving_path;
            return {};
        }

        serving_path_set.insert(serving_path);

        nginx_config_map location_configs = {};
        if (child_block != nullptr) {
            location_configs = unroll_one_level(child_block->statements_);
        }

        if (handler_name == "EchoHandler") {
            payload.handlers.push_back(std::make_shared<EchoHandler>(serving_path));

        } else if (handler_name == "StaticHandler" && location_configs.count("root")) {
            const auto &root = location_configs.at("root").at(0)->tokens_;

            if (root.size() != 2) {
                BOOST_LOG_TRIVIAL(warning) << "Skipping location handler with bad root.";
                BOOST_LOG_TRIVIAL(info) << loc_items->ToString(0);
                continue;
            }

            payload.handlers.push_back(
                std::make_shared<StaticFileHandler>(serving_path, root.at(1)));

        } else {
            BOOST_LOG_TRIVIAL(warning)
                << "Skipping bad handler: " << handler_name << " at " << serving_path;
            continue;
        }
    }

    if (payload.handlers.empty()) {
        BOOST_LOG_TRIVIAL(warning) << "No valid handlers found. Defaulting to EchoHandler.";
        payload.handlers.push_back(std::make_shared<EchoHandler>("/"));
    }

    return payload;
}

/// @brief unfolds one level of an Nginx statement such that the result
/// @brief 1. can be looked up easily by key
/// @brief 2. preserves repeated statements (aggregates them on the same key)
/// @brief 3. systematizes the process of making a lookup
/// @param statement statements of root node you want to unfold
/// @return a pair containing leaf nodes and nested nodes
static nginx_config_map unroll_one_level(const std::vector<nginx_shared_statement> &statements) {
    nginx_config_map result;

    for (auto const &child : statements) {
        const auto &this_block = child->child_block_;
        // first token is always guaranteed
        auto first_token = child->tokens_.at(0);
        std::vector<std::string> rest_tokens = {};
        if (!result.count(first_token)) {
            result.insert({first_token, {}});
        }
        auto &token = result.at(first_token);
        token.push_back(child);
    }
    return result;
}

static bool string_is_quoted(std::string text) {
    if (text.size() < 2) {
        return false;
    }
    std::stringstream quotes;
    quotes << text.front() << text.back();
    return quotes.str() == "\"\"" || quotes.str() == "''";
}
