#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <functional>

#include "session.h"


using boost::asio::ip::tcp;
//Defines a factory function type which returns a pointer to a session
using SessionFactory = std::function<session*(boost::asio::io_service&)>;

class server {
public:
//Refactored the constructor so that we have a default session unless we give a mock session
  server(boost::asio::io_service& io_service, short port,
    SessionFactory factory = [](auto& io) { return new session(io); });

protected:
//Moved to protected since cpp does not allow use of private functions out of class
  void handle_accept(session* new_session,
      const boost::system::error_code& error);
  void start_accept();

private:
  boost::asio::io_service& io_service_;
  tcp::acceptor acceptor_;
  SessionFactory session_factory_;
};

#endif