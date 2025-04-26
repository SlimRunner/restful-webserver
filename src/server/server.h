#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <vector>

#include "request_handlers/request_handler.h"
#include "session.h"

using boost::asio::ip::tcp;
// Defines a factory function type which returns a pointer to a session
using SessionFactory = std::function<session *(boost::asio::io_service &,
                                               std::vector<std::shared_ptr<RequestHandler>>)>;

class server {
   public:
    // Refactored the constructor so that we have a default session unless we give a mock session
    server(
        boost::asio::io_service &io_service, short port,
        std::vector<std::shared_ptr<RequestHandler>> handlers,
        SessionFactory factory = [](auto &io, auto handlers) { return new session(io, handlers); });

   protected:
    // Moved to protected since cpp does not allow use of private functions out of class
    void handle_accept(session *new_session, const boost::system::error_code &error);
    void start_accept();

   private:
    boost::asio::io_service &io_service_;
    tcp::acceptor acceptor_;
    SessionFactory session_factory_;
    std::vector<std::shared_ptr<RequestHandler>> handlers_;
};

#endif
