#include <iostream>
#include <boost/log/utility/setup/console.hpp>

#include <boost/json.hpp>

#include "logger.hpp"

void ConsoleJsonFormatter(logging::record_view const& rv, logging::formatting_ostream & fos) {

    json::object log_message_obj;

    // auto ts = *rv[timestamp];
    // log_message_obj["timestamp"] = to_iso_extended_string(ts);
    log_message_obj["timestamp"] = to_iso_extended_string(boost::posix_time::second_clock::local_time());

    log_message_obj["data"] = (rv[data_])->as_object();
    log_message_obj["message"] = *rv[message_];

    fos << json::value(log_message_obj);
}

void SetupConsoleLogging() {

    logging::add_console_log(
        std::cout,
        keywords::format = &ConsoleJsonFormatter,
        keywords::auto_flush = true
    );
}