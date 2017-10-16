/*
  Copyright 2017 Mathieu Bernard

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "logging.hh"

#include <iostream>
#include <stdexcept>

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>


logging::severity logging::m_level = logging::severity::warning;


const std::map<std::string, logging::severity> logging::m_level_name =
{
    {"fatal", severity::fatal},
    {"error", severity::error},
    {"warning", severity::warning},
    {"info", severity::info},
    {"debug", severity::debug},
    {"trace", severity::trace}
};


// more on https://gist.github.com/xiongjia/e23b9572d3fc3d677e3d
void logging::init(const logging::severity level)
{
    // ignore messages above m_level
    boost::log::add_common_attributes();
    boost::log::core::get()->set_filter(
        boost::log::trivial::severity >= boost::ref(logging::m_level));

    // log formatter: [TimeStamp] [ThreadId] [Level] Log message
    auto timestamp = boost::log::expressions::
        format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f");

    auto thread = boost::log::expressions::
        attr<boost::log::attributes::current_thread_id::value_type>("ThreadID");

    auto severity = boost::log::expressions::
        attr<boost::log::trivial::severity_level>("Severity");

    boost::log::formatter formatter =
        boost::log::expressions::format("[%1%] (%2%) [%3%] %4%")
        % timestamp % thread % severity % boost::log::expressions::smessage;

    // output to stderr
    auto console_sink = boost::log::add_console_log();
    console_sink->set_formatter(formatter);
    set_level(level);
}


void logging::init(const std::string level)
{
    try
    {
        init(m_level_name.at(level));
    }
    catch (std::out_of_range)
    {
        init(severity::debug);
        LOG(error) << "error in logging::init, " << level
                   << " is not a valid severity level, fall back to debug";
    }
}


void logging::set_level(const logging::severity level)
{
    m_level = level;
    LOG(info) << "set log level to " << m_level;
}


void logging::set_level(const std::string level)
{
    try
    {
        m_level = m_level_name.at(level);
        LOG(info) << "set log level to " << m_level;
    }
    catch (std::out_of_range)
    {
        init(severity::debug);
        LOG(error) << "error in logging::init, " << level
                   << " is not a valid severity level, fall back to debug";
    }
}


logging::severity logging::get_level()
{
    return m_level;
}
