#include "logger.h"
// Including libraries to have Thread-safe logging,
// Console + file output, Log rotation, Metadata (timestamp, thread ID)
// and Easy setup functions
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/keywords/max_files.hpp>
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
    constexpr std::size_t MAX_LOG_SIZE = 10 * 1024 * 1024;         // 10 MB
    constexpr std::size_t LOG_COUNT_LIMIT = 100;                   // 100 files
    constexpr std::size_t LOG_DIR_SIZE_LIMIT = 10 * MAX_LOG_SIZE;  // 100 MB
    // Console sink
    logging::add_console_log(
        std::clog, keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%]: %Message%");

    // File sink with rotation and limited number of files
    auto fileSink = logging::add_file_log(
        // pattern for rotated files
        keywords::file_name = "logs/server_%Y-%m-%d_%N.log",
        // rotate every `MAX_LOG_SIZE` bytes
        keywords::rotation_size = MAX_LOG_SIZE,
        // rotate at midnight
        // parameters are (Hour, Minute, Second)
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
        // flushes every log write. useful in short tests
        keywords::auto_flush = true,
        // ensures append mode is used
        keywords::open_mode = std::ios_base::app,
        keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%]: %Message%",
        // enables the backend to delete old logs using specified collector
        keywords::scan_method = sinks::file::scan_matching);

    // sets the directory `target` to `./logs`
    // sets the `max_size` limit for all logs to `MAX_TOTAL_SIZE`
    //
    fileSink->locked_backend()->set_file_collector(sinks::file::make_collector(
        keywords::target = "logs", keywords::max_size = LOG_DIR_SIZE_LIMIT,
        keywords::max_files = LOG_COUNT_LIMIT));

    // on startup, scan the target directory for existing files
    fileSink->locked_backend()->scan_for_files();

    // Add common attributes like TimeStamp and ThreadID
    logging::add_common_attributes();
}
