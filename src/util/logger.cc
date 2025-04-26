#include "logger.h"
// Including libraries to have Thread-safe logging,
// Console + file output, Log rotation, Metadata (timestamp, thread ID)
// and Easy setup functions
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

void init_logging() {
    // Console sink
    logging::add_console_log(
        std::clog, keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%]: %Message%");

    // File sink with rotation
    logging::add_file_log(
        keywords::file_name = "logs/server_%Y-%m-%d.log",
        keywords::rotation_size = 10 * 1024 * 1024,  // 10 MB
        // setting the rotation time to midnight
        // Creates a rotation time point of every day at the specified time
        // parameters are (Hour, Minute, Second)
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
        keywords::auto_flush = true,               // flushes every log write. useful in short tests
        keywords::open_mode = std::ios_base::app,  // this ensures append mode
        keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%]: %Message%");

    // Add common attributes like TimeStamp and ThreadID
    logging::add_common_attributes();
}
