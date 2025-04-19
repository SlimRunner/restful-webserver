#ifndef SESSION_H
#define SESSION_H

#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class session {
public:
  session(boost::asio::io_service& io_service);
  tcp::socket& socket();
  //Added a virtual destructor
  virtual ~session() = default; 
  //Made the start method virtual so can be overriden in tests
  virtual void start();
  std::string build_response(const std::string& full_request);
  void set_request(const std::string& req);

private:
  void handle_read(const boost::system::error_code& error,
      size_t bytes_transferred);
  void handle_write(const boost::system::error_code& error);
  void safe_delete();
  
  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
  std::string request_buffer_;
  bool already_deleted_ = false;
  // max request size = 8 KB = 8192 bytes to prevent buffer overflow
  static const size_t max_request_size_ = 8192; 

};

#endif
