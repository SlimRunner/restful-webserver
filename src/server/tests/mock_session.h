#ifndef MOCK_SESSION_H
#define MOCK_SESSION_H

#include <iostream>

#include "session.h"

class MockSession : public session {
   public:
    MockSession(boost::asio::io_service& io_service) : session(io_service), start_called_(false) {}

    void start() override {
        start_called_ = true;
    }

    bool start_called() const {
        return start_called_;
    }

   private:
    bool start_called_;
};

#endif
