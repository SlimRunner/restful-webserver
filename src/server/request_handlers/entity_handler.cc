#include "entity_handler.h"
#include "real_filesystem.h"
#include "not_found_exception.h"
#include "invalid_id_exception.h"
#include "file_io_exception.h"

#include <boost/log/trivial.hpp>
#include <sstream>

volatile int force_link_entity_handler = 0;

EntityHandler::EntityHandler(
    const std::string& path_prefix,
    const std::map<std::string, std::string>& args)
    : path_prefix_(path_prefix)
{
    auto it = args.find("root");
    if (it == args.end()) {
        throw std::runtime_error("EntityHandler requires a 'root' argument.");
    }

    base_dir_ = it->second;
    BOOST_LOG_TRIVIAL(debug) << "EntityHandler constructed with path_prefix: " << path_prefix_ << ", base_dir: " << base_dir_;
}

void EntityHandler::set_filesystem(std::unique_ptr<Filesystem> fs) {
    BOOST_LOG_TRIVIAL(debug) << "Injecting custom filesystem";
    fs_ = std::move(fs);
}

Filesystem* EntityHandler::get_filesystem() const {
    return fs_.get();
}

std::unique_ptr<HttpResponse> EntityHandler::handle_request(const HttpRequest& request)
{
    if (!fs_) {
        BOOST_LOG_TRIVIAL(debug) << "Initializing RealFilesystem for base_dir: " << base_dir_;
        fs_ = std::make_unique<RealFilesystem>(base_dir_);
    }

    BOOST_LOG_TRIVIAL(info) << "Handling request: method=" << request.method << ", path=" << request.path;
    HttpResponse response;

    try {
        std::string path = request.path;

        std::string entity, id;

        // Extract entity from path_prefix_ (e.g. "/api/entity" => "entity")
        size_t first = path.find('/', 0); // skip initial '/'
        size_t second = path.find('/', first + 1);
        size_t third = path.find('/', second + 1);
        if (second != std::string::npos && third != std::string::npos) {
            entity = path.substr(second + 1, third - second - 1);
            id = path.substr(third + 1);
        } else {
            entity = path.substr(second + 1);
        }

        BOOST_LOG_TRIVIAL(debug) << "Parsed entity: " << entity << ", id: " << id;

        if (request.method == "POST") {
            std::string new_id = fs_->next_id(entity);
            BOOST_LOG_TRIVIAL(debug) << "Data is " << request.body;
            fs_->write(entity, new_id, request.body);
            response.status = StatusCode::OK;
            response.body = "{\"id\": \"" + new_id + "\"}";
            response.headers["Content-Type"] = "application/json";
            response.headers["Content-Length"] = std::to_string(response.body.size());
            BOOST_LOG_TRIVIAL(debug) << "Created new ID: " << new_id << " for entity: " << entity;
        } else if (request.method == "GET") {
            if (id.empty()) {
                auto ids = fs_->list_ids(entity);
                std::ostringstream oss;
                oss << "[";
                for (size_t i = 0; i < ids.size(); ++i) {
                    oss << "\"" << ids[i] << "\"";
                    if (i < ids.size() - 1) oss << ",";
                }
                oss << "]";
                response.status = StatusCode::OK;
                response.body = oss.str();
                response.headers["Content-Length"] = std::to_string(response.body.size());
                response.headers["Content-Type"] = "application/json";
                BOOST_LOG_TRIVIAL(debug) << "Listed IDs for entity: " << entity;
            } else {
                response.body = fs_->read(entity, id);
                response.status = StatusCode::OK;
                response.headers["Content-Length"] = std::to_string(response.body.size());
                response.headers["Content-Type"] = "application/json";
                BOOST_LOG_TRIVIAL(debug) << "Read data for entity: " << entity << ", id: " << id;
            }
        } else if (request.method == "PUT") {
            if (id.empty()) {
                throw std::runtime_error("PUT request must specify an ID.");
            }
            fs_->write(entity, id, request.body);
            response.status = StatusCode::OK;
            response.body = "{\"id\": \"" + id + "\"}";
            response.headers["Content-Type"] = "application/json";
            response.headers["Content-Length"] = std::to_string(response.body.size());
            BOOST_LOG_TRIVIAL(debug) << "Wrote data for entity: " << entity << ", id: " << id;
        } else if (request.method == "DELETE") {
            if (id.empty()) {
                throw std::runtime_error("DELETE request must specify an ID.");
            }
            fs_->remove(entity, id);
            response.body = "{\"status\": \"deleted\"}";
            response.headers["Content-Type"] = "application/json";
            response.headers["Content-Length"] = std::to_string(response.body.size());
            response.status = StatusCode::OK;
            BOOST_LOG_TRIVIAL(debug) << "Removed entity: " << entity << ", id: " << id;
        } else {
            response.status = StatusCode::BAD_REQUEST;
            response.body = "Unsupported HTTP method.";
            response.headers["Content-Length"] = std::to_string(response.body.size());
            BOOST_LOG_TRIVIAL(warning) << "Unsupported method: " << request.method;
        }
    } catch (const NotFoundException& e) {
        response.status = StatusCode::NOT_FOUND;
        response.body = e.what();
        response.headers["Content-Length"] = std::to_string(response.body.size());
        BOOST_LOG_TRIVIAL(warning) << "NotFoundException: " << e.what();
    } catch (const InvalidIdException& e) {
        response.status = StatusCode::BAD_REQUEST;
        response.body = e.what();
        response.headers["Content-Length"] = std::to_string(response.body.size());
        BOOST_LOG_TRIVIAL(warning) << "InvalidIdException: " << e.what();
    } catch (const FileIOException& e) {
        response.status = StatusCode::INTERNAL_SERVER_ERROR;
        response.body = e.what();
        response.headers["Content-Length"] = std::to_string(response.body.size());
        BOOST_LOG_TRIVIAL(error) << "FileIOException: " << e.what();
    } catch (const std::exception& e) {
        response.status = StatusCode::BAD_REQUEST;
        response.body = e.what();
        response.headers["Content-Length"] = std::to_string(response.body.size());
        BOOST_LOG_TRIVIAL(error) << "std::exception: " << e.what();
    }

    return std::make_unique<HttpResponse>(response);
}


/*
This program is ran at startup, before main()
It lets the registry know that if we need EntityHandler it will start building it
*/
#include "handler_registry.h"
REGISTER_HANDLER_WITH_NAME(EntityHandler, "EntityHandler")
