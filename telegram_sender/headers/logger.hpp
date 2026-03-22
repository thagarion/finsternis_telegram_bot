#pragma once

#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>

class Logger final {
    Logger() = default;

    ~Logger() = default;

public:
    Logger(const Logger &) = delete;

    Logger &operator=(const Logger &) = delete;

    static void Init(int level = 2) {
        boost::log::add_common_attributes();
        boost::log::core::get()->set_filter(boost::log::trivial::severity >= level);

        auto fmt_time_stamp =
                boost::log::expressions::format_date_time<
                    boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f");
        auto fmt_thread_id =
                boost::log::expressions::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID");
        auto fmt_severity = boost::log::expressions::attr<boost::log::trivial::severity_level>("Severity");

        const boost::log::formatter log_format =
                boost::log::expressions::format("[%1%] (%2%) [%3%] %4%") % fmt_time_stamp %
                fmt_thread_id % fmt_severity % boost::log::expressions::smessage;

        // console sink
        const auto console_sink = boost::log::add_console_log(std::cout);
        console_sink->set_formatter(log_format);

        // fs sink
        const auto fs_sink = boost::log::add_file_log(boost::log::keywords::file_name = "logs/%Y-%m-%d_%H-%M-%S.%N.log",
                                                      boost::log::keywords::rotation_size = 10 * 1024 * 1024,
                                                      boost::log::keywords::min_free_space = 30 * 1024 * 1024,
                                                      boost::log::keywords::open_mode = std::ios_base::app);
        fs_sink->set_formatter(log_format);
        fs_sink->locked_backend()->auto_flush(true);
    }
};

#define LOG(level, arg) BOOST_LOG_TRIVIAL(level) << "[" << __FILE__ << ":" << __LINE__ << "] " << arg;

#define LOG_TRACE(arg) LOG(trace, arg);
#define LOG_DEBUG(arg) LOG(debug, arg);
#define LOG_INFO(arg) LOG(info, arg);
#define LOG_WARN(arg) LOG(warning, arg);
#define LOG_ERROR(arg) LOG(error, arg);
#define LOG_FATAL(arg) LOG(fatal, arg);