# Copyright 2017 Mathieu Bernard
#
# This file is part of wordseg: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# wordseg is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with wordseg. If not, see <http://www.gnu.org/licenses/>.


# Building script for the dpseg program

# looking for boost libraries
find_package(Boost REQUIRED COMPONENTS program_options)

# compiling a C++11 program. NDEBUG flag is to avoid the compilation
# of debugging code
set(CMAKE_CXX_FLAGS "-std=c++11 -DNDEBUG")

# the source files to compile
file(GLOB SOURCES src/*.cc include/*)

add_executable(dpseg ${SOURCES})
include_directories(include ${Boost_INCLUDE_DIRS})
target_link_libraries(dpseg ${Boost_LIBRARIES})
