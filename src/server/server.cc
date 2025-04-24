#include "server.h"
#include <boost/bind/bind.hpp>
using namespace boost::placeholders;

server::server(boost::asio::io_service &io_service, short port,
               std::vector<std::shared_ptr<RequestHandler>> handlers,
               SessionFactory factory)
    : io_service_(io_service),
      acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
      // Save the provided factory function so it can be used later in start_accept()
      session_factory_(factory),
      handlers_(handlers)
{
  start_accept();
}

void server::start_accept()
{
  // session* new_session = new session(io_service_);
  // This is where we use the injected factory whether it be real or mock
  session *new_session = session_factory_(io_service_, handlers_);
  acceptor_.async_accept(new_session->socket(),
                         boost::bind(&server::handle_accept, this, new_session,
                                     boost::asio::placeholders::error));
}

void server::handle_accept(session *new_session, const boost::system::error_code &error)
{
  if (!error)
  {
    new_session->start();
  }
  else
  {
    delete new_session;
  }

  start_accept();
}