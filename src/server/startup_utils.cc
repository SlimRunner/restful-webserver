#include "startup_utils.h"

#include <boost/log/trivial.hpp>
#include <future>
#include <set>
#include <thread>
#include <utility>
#include <vector>

#include "handler_registry.h"
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
        payload.route_map.insert({"/", {"EchoHandler", {}}});
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

        if (serving_path.length() > 1 && serving_path.back() == '/') {
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

        RoutingPayload routeMetadata = {};
        routeMetadata.handler = handler_name;

        // Build argument map from config block
        for (const auto &[key, stmts] : location_configs) {
            for (const auto &stmt : stmts) {
                const auto &tokens = stmt->tokens_;
                if (tokens.size() == 2) {
                    BOOST_LOG_TRIVIAL(debug)
                        << key << ": " << "" << tokens.at(0) << "->" << tokens.at(1);
                    routeMetadata.arguments.insert({tokens.at(0), tokens.at(1)});
                } else {
                    BOOST_LOG_TRIVIAL(info)
                        << "Skipped malformed config " << "with more than one value. "
                        << "Handler name " << handler_name << " served at " << serving_path;
                }
            }
        }

        payload.route_map.insert({serving_path, routeMetadata});
        BOOST_LOG_TRIVIAL(info) << "Handler mapped: " << handler_name << " at " << serving_path;
    }

    if (payload.route_map.empty()) {
        BOOST_LOG_TRIVIAL(warning) << "No valid handlers found. Defaulting to EchoHandler.";
        payload.route_map.insert({"/", {"EchoHandler", {}}});
    }

    return payload;
}

void init_threads(boost::asio::io_service &io_service, int num_threads) {
    boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);

    // When signal received, stop the io_service
    signals.async_wait([&](const boost::system::error_code &error, int signal_number) {
        if (!error) {
            BOOST_LOG_TRIVIAL(info)
                << "Received signal " << signal_number << ". Initiating graceful shutdown...";
            // Post to the io_service to stop safely
            io_service.stop();
        }
    });

    std::vector<std::thread> thread_pool;
    std::vector<std::future<void>> except_futures;
    except_futures.reserve(num_threads);

    BOOST_LOG_TRIVIAL(info) << "Starting " << num_threads << " worker threads...";

    for (int i = 0; i < num_threads; ++i) {
        // promise is local but its data is moved to the worker thread
        // so it does not die after this loop
        std::promise<void> p;
        except_futures.push_back(p.get_future());

        thread_pool.emplace_back(
            [&io_service, i](std::promise<void> promise) {
                try {
                    BOOST_LOG_TRIVIAL(debug) << "Thread " << i << " started";
                    io_service.run();
                    BOOST_LOG_TRIVIAL(debug) << "Thread " << i << " exited cleanly";
                    promise.set_value();  // Fulfill the promise on success

                } catch (const std::exception &e) {
                    BOOST_LOG_TRIVIAL(error) << "Exception in thread " << i << ": " << e.what();

                    // this allows main to know about a thread error
                    promise.set_exception(std::current_exception());
                }
            },
            std::move(p));
    }

    for (auto &t : thread_pool) {
        // this blocks until thread t is done. We do not need any
        // specific order of completion so this is fine.
        t.join();
    }

    // Check for exceptions
    for (int i = 0; i < num_threads; ++i) {
        // re-throw exception from thread. This only throws the first
        // one encountered, but the rest are logged prior. It is main's
        // job to catch this.
        except_futures[i].get();
    }

    BOOST_LOG_TRIVIAL(info) << "All worker threads have exited. Server shutdown complete.";
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
