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

#ifndef _LOG_HH
#define _LOG_HH

/*
   Logging facilities, wrapper on boost::log::trivial

   The possible severity levels for messages are fatal, error,
   warning, info, debug and trace. Once included, use as follow:

   logging::init(info);
   LOG(warning) << "this is a warning";
   LOG(debug) << "ignored because debug < info";

   logging::set_level(logging::severity::debug);
   LOG(debug) << "now debug is logged out";

   logging::severity my_level = logging::get_level();
 */


#include <map>
#include <string>

#define BOOST_LOG_DYN_LINK
#include <boost/log/trivial.hpp>


#define LOG BOOST_LOG_TRIVIAL

class logging {
public:
    // the enum type of severity levels
    using severity = boost::log::trivial::severity_level;

    // initialize the logger at the given `level`
    static void init(const severity level = logging::severity::warning);

    // initialize the logger at the given `level`
    static void init(const std::string level = "warning");

    // set the severity level
    static void set_level(const severity level);

    // get the severity level
    static severity get_level();

private:
    // the logging severity level
    static severity m_level;

    // a mapping "level" -> level to implement logging::init(std::string)
    static const std::map<std::string, severity> m_level_name;
};

#endif  // _LOG_HH
