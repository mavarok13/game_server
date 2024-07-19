#pragma once
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/expressions.hpp>
#include <boost/date_time.hpp>
#include <boost/json.hpp>

namespace logging = boost::log;
namespace keywords = logging::keywords;

namespace json = boost::json;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime);
BOOST_LOG_ATTRIBUTE_KEYWORD(data_, "Data_", json::value);
BOOST_LOG_ATTRIBUTE_KEYWORD(message_, "Message_", std::string);

void SetupConsoleLogging();

void ConsoleJsonFormatter(logging::record_view const& rv, logging::formatting_ostream & fos);